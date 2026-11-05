#pragma once
#include <windows.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs=std::filesystem;
struct Image { int w=0,h=0; std::vector<unsigned char> rgb; };
inline fs::path baseDir(){wchar_t p[32768]; GetModuleFileNameW(nullptr,p,32768);return fs::path(p).parent_path();}
inline std::wstring choose(HWND owner){wchar_t p[32768]={};OPENFILENAMEW o={sizeof(o)};o.hwndOwner=owner;o.lpstrFilter=L"图像文件\0*.bmp;*.png;*.jpg;*.jpeg\0所有文件\0*.*\0";o.lpstrFile=p;o.nMaxFile=32768;o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;return GetOpenFileNameW(&o)?p:L"";}
inline Image loadImage(const fs::path& path){
 Gdiplus::Bitmap b(path.c_str());if(b.GetLastStatus()!=Gdiplus::Ok)throw std::runtime_error("Cannot read image");
 if(!b.GetWidth()||!b.GetHeight()||b.GetWidth()>4096||b.GetHeight()>4096)throw std::runtime_error("Image dimensions must be 1..4096");
 Image im{int(b.GetWidth()),int(b.GetHeight()),{}};im.rgb.resize(size_t(im.w)*im.h*3);
 Gdiplus::Rect r(0,0,im.w,im.h);Gdiplus::BitmapData d{};
 if(b.LockBits(&r,Gdiplus::ImageLockModeRead,PixelFormat24bppRGB,&d)!=Gdiplus::Ok)throw std::runtime_error("Cannot decode pixels");
 for(int y=0;y<im.h;++y)for(int x=0;x<im.w;++x)for(int c=0;c<3;++c)im.rgb[(size_t(y)*im.w+x)*3+c]=((unsigned char*)d.Scan0)[ptrdiff_t(y)*d.Stride+x*3+2-c];
 b.UnlockBits(&d);return im;
}
inline void saveBmp(const Image& im,const fs::path& path){
 int stride=(im.w*3+3)&~3;BITMAPFILEHEADER f{};BITMAPINFOHEADER h{};f.bfType=0x4d42;f.bfOffBits=54;f.bfSize=54+stride*im.h;h.biSize=40;h.biWidth=im.w;h.biHeight=im.h;h.biPlanes=1;h.biBitCount=24;h.biSizeImage=stride*im.h;
 std::ofstream out(path,std::ios::binary);out.write((char*)&f,14);out.write((char*)&h,40);std::vector<unsigned char> row(stride);
 for(int y=im.h-1;y>=0;--y){for(int x=0;x<im.w;++x)for(int c=0;c<3;++c)row[x*3+c]=im.rgb[(size_t(y)*im.w+x)*3+2-c];out.write((char*)row.data(),stride);}if(!out)throw std::runtime_error("Cannot write BMP");
}
inline void draw(HDC dc,const Image& im,int x,int y,int w,int h){if(im.rgb.empty())return;int stride=(im.w*3+3)&~3;std::vector<unsigned char> b(size_t(stride)*im.h);for(int j=0;j<im.h;++j)for(int i=0;i<im.w;++i)for(int c=0;c<3;++c)b[size_t(j)*stride+i*3+c]=im.rgb[(size_t(j)*im.w+i)*3+2-c];BITMAPINFO info{};info.bmiHeader.biSize=40;info.bmiHeader.biWidth=im.w;info.bmiHeader.biHeight=-im.h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=24;double s=std::min(double(w)/im.w,double(h)/im.h);int dw=int(im.w*s),dh=int(im.h*s);SetStretchBltMode(dc,COLORONCOLOR);StretchDIBits(dc,x+(w-dw)/2,y+(h-dh)/2,dw,dh,0,0,im.w,im.h,b.data(),&info,DIB_RGB_COLORS,SRCCOPY);}
inline HWND ctl(HWND w,const wchar_t* cls,const wchar_t* text,int id,int x,int y,int width,int height,DWORD style=0){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return c;}
inline void error(HWND w,const std::exception& e){int n=MultiByteToWideChar(CP_UTF8,0,e.what(),-1,nullptr,0);std::wstring s(n,0);MultiByteToWideChar(CP_UTF8,0,e.what(),-1,s.data(),n);MessageBoxW(w,s.c_str(),L"操作提示",MB_ICONERROR);}
inline int number(HWND w,int id,int lo,int hi){wchar_t s[32];GetDlgItemTextW(w,id,s,32);wchar_t* end;long n=wcstol(s,&end,10);if(end==s||*end||n<lo||n>hi)throw std::runtime_error("Numeric value out of range");return int(n);}
inline int runWindow(HINSTANCE h,const wchar_t* cls,const wchar_t* title,WNDPROC proc,int width,int height){WNDCLASSW c{};c.hInstance=h;c.lpszClassName=cls;c.lpfnWndProc=proc;c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);HWND w=CreateWindowW(cls,title,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,width,height,nullptr,nullptr,h,nullptr);if(!w)return 1;ShowWindow(w,SW_SHOW);MSG m;while(GetMessageW(&m,nullptr,0,0)>0){TranslateMessage(&m);DispatchMessageW(&m);}return int(m.wParam);}
