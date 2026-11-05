#include "image_common.h"
Image a,b,current;int frame=0,steps=20,interval=100;bool playing=false;fs::path pa,pb;
Image blend(const Image& a,const Image& b,int k,int n){
 if(a.w!=b.w||a.h!=b.h||a.rgb.empty())throw std::runtime_error("Start and end image sizes must match");
 if(n<1||k<0||k>n)throw std::runtime_error("Invalid frame index");
 Image out=a;for(size_t i=0;i<out.rgb.size();++i)out.rgb[i]=static_cast<unsigned char>(((n-k)*int(a.rgb[i])+k*int(b.rgb[i])+n/2)/n);return out;
}
void status(HWND w){std::wstring s=L"当前帧 "+std::to_wstring(frame)+L" / "+std::to_wstring(steps)+L"    α = "+std::to_wstring(double(frame)/steps).substr(0,5)+L"    "+(playing?L"正在播放":L"已停止 / 暂停");SetDlgItemTextW(w,20,s.c_str());InvalidateRect(w,nullptr,TRUE);}
void stop(HWND w){KillTimer(w,1);playing=false;}
void check(){if(a.rgb.empty()||b.rgb.empty())throw std::runtime_error("Please load both BMP images");if(a.w!=b.w||a.h!=b.h)throw std::runtime_error("Start and end image sizes must match");}
void exportFrames(const fs::path& dir,int n){fs::create_directories(dir);for(int k=0;k<=n;++k){wchar_t name[64];swprintf(name,64,L"frame_%03d.bmp",k);saveBmp(blend(a,b,k,n),dir/name);}std::ofstream f(dir/L"参数.txt");f<<"intervals="<<n<<"\nframes="<<n+1<<"\nwidth="<<a.w<<"\nheight="<<a.h<<"\n";if(!f)throw std::runtime_error("Cannot write parameters");}
LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){try{switch(m){case WM_CREATE:
 ctl(w,L"BUTTON",L"打开起始帧",1,20,18,125,30);ctl(w,L"BUTTON",L"打开终止帧",2,155,18,125,30);
 ctl(w,L"STATIC",L"过渡间隔数",0,295,23,85,22);ctl(w,L"EDIT",L"20",10,385,18,60,28,WS_BORDER|ES_NUMBER);
 ctl(w,L"STATIC",L"帧间隔 ms",0,460,23,80,22);ctl(w,L"EDIT",L"100",11,545,18,65,28,WS_BORDER|ES_NUMBER);
 ctl(w,L"BUTTON",L"播放 / 重播",3,625,18,120,30);ctl(w,L"BUTTON",L"暂停 / 继续",4,755,18,115,30);ctl(w,L"BUTTON",L"导出 BMP 序列",5,880,18,145,30);
 ctl(w,L"STATIC",L"起始帧",21,20,65,1000,24);ctl(w,L"STATIC",L"终止帧",22,20,92,1000,24);ctl(w,L"STATIC",L"请打开两幅相同尺寸的 BMP 图像",20,20,126,1000,25);
 if(fs::exists(baseDir()/L"实验素材/起始帧1.bmp")){pa=baseDir()/L"实验素材/起始帧1.bmp";pb=baseDir()/L"实验素材/终止帧1.bmp";a=loadImage(pa);b=loadImage(pb);current=a;SetDlgItemTextW(w,21,(L"起始帧："+pa.filename().wstring()).c_str());SetDlgItemTextW(w,22,(L"终止帧："+pb.filename().wstring()).c_str());}return 0;
 case WM_COMMAND:switch(LOWORD(wp)){case 1:case 2:{auto p=choose(w);if(p.empty())break;if(fs::path(p).extension()!=L".bmp"&&fs::path(p).extension()!=L".BMP")throw std::runtime_error("Please select a BMP file");Image im=loadImage(p);stop(w);if(LOWORD(wp)==1){a=std::move(im);pa=p;current=a;frame=0;}else{b=std::move(im);pb=p;}SetDlgItemTextW(w,LOWORD(wp)==1?21:22,(std::wstring(LOWORD(wp)==1?L"起始帧：":L"终止帧：")+fs::path(p).filename().wstring()).c_str());status(w);break;}
 case 3:check();steps=number(w,10,1,300);interval=number(w,11,10,5000);stop(w);frame=0;current=a;playing=SetTimer(w,1,interval,nullptr)!=0;if(!playing)throw std::runtime_error("Cannot create timer");status(w);break;
 case 4:check();if(playing)stop(w);else if(frame<steps){playing=SetTimer(w,1,interval,nullptr)!=0;if(!playing)throw std::runtime_error("Cannot create timer");}status(w);break;
 case 5:check();{int n=number(w,10,1,300);auto dir=baseDir()/L"实验结果"/(L"帧序列_"+std::to_wstring(n)+L"段");exportFrames(dir,n);SetDlgItemTextW(w,20,(L"已导出 "+std::to_wstring(n+1)+L" 帧至 "+dir.wstring()).c_str());}break;}return 0;
 case WM_TIMER:if(playing){++frame;current=blend(a,b,frame,steps);if(frame==steps)stop(w);status(w);}return 0;
 case WM_PAINT:{PAINTSTRUCT p;HDC dc=BeginPaint(w,&p);draw(dc,a,20,190,325,360);draw(dc,current,365,190,325,360);draw(dc,b,710,190,325,360);TextOutW(dc,145,165,L"起始图像",4);TextOutW(dc,485,165,L"当前插值帧",5);TextOutW(dc,840,165,L"终止图像",4);EndPaint(w,&p);return 0;}
 case WM_DESTROY:stop(w);PostQuitMessage(0);return 0;}}catch(const std::exception& e){stop(w);error(w,e);}return DefWindowProcW(w,m,wp,lp);}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,LPWSTR,int){ULONG_PTR token;Gdiplus::GdiplusStartupInput g;if(Gdiplus::GdiplusStartup(&token,&g,nullptr)!=Gdiplus::Ok)return 1;int ret=0;try{std::wstring cmd=GetCommandLineW();if(cmd.find(L"--self-test")!=std::wstring::npos){Image x{1,1,{0,100,255}},y{1,1,{255,0,0}};if(blend(x,y,0,20).rgb!=x.rgb||blend(x,y,20,20).rgb!=y.rgb||blend(x,y,10,20).rgb!=std::vector<unsigned char>({128,50,128}))throw std::runtime_error("Interpolation self test failed");bool rejected=false;try{blend(x,Image{2,1,{0,0,0,0,0,0}},1,2);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("Dimension validation failed");a=loadImage(baseDir()/L"实验素材/起始帧1.bmp");b=loadImage(baseDir()/L"实验素材/终止帧1.bmp");exportFrames(baseDir()/L"实验结果/帧序列_20段",20);auto first=loadImage(baseDir()/L"实验结果/帧序列_20段/frame_000.bmp");auto last=loadImage(baseDir()/L"实验结果/帧序列_20段/frame_020.bmp");if(first.rgb!=a.rgb||last.rgb!=b.rgb)throw std::runtime_error("BMP roundtrip failed");std::ofstream(baseDir()/L"实验结果/自检.txt")<<"PASS: endpoints, midpoint, size rejection, BMP roundtrip; 21 frames exported.\n";}else ret=runWindow(h,L"ImageMorphLab",L"实验3.1 图像变换动画",proc,1070,620);}catch(const std::exception& e){std::ofstream(baseDir()/L"实验结果/错误.txt")<<e.what();ret=1;}Gdiplus::GdiplusShutdown(token);return ret;}
