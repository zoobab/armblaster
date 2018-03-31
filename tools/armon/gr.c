//	====================================================
//	�b����:�ȈՃO���t�B�b�N���C�u����(Win32��) ������
//	====================================================

#include <math.h>
#include "gr.h"

#ifndef	_LINUX_
//	====================================================
//	���[�J����`.
//	====================================================
#define	u_int	unsigned int
#define SWAP(x,y)  {int t; t=(x); (x)=(y); (y)=t; }

//	====================================================
//	���C�u�����R���e�L�X�g.
//	====================================================
typedef	struct {
	u_int		width,height;	//�`��̈�̃T�C�Y.
	char		menuname[256];	//�E�B���h�E�^�C�g��.

	int	 		mouse_x,mouse_y;//�}�E�X���W.
	int 	   *pixbuf;			//�`��o�b�t�@.

	HWND        hwMain ;		//CreateWindow�����E�B���h�E�n���h��.
	HINSTANCE 	instance ;		//GetModuleHandle(NULL)���������̃C���X�^���X.
	HBITMAP 	hBMP;			//CreateDIBSection()�����r�b�g�}�b�v.
	HDC			hdcMem ;		//�������[�c�b(�f�o�C�X�R���e�L�X�g)
	LPBYTE		lpBuf ;			//GlobalAlloc()���� BitMapInfo �\����.

	// ��ԃt���O.
	int  		destroy_flag ;	// 1�Ȃ�ADestroyWindow()���L�b�N������.
	int	 		esc_flag;		// 1�Ȃ�AESC�L�[�������ꂽ.
	int	 		end_flag;		// 1�Ȃ�AWinMain���I���i���邢�͗\���j
	int	 		close_flag;		// 1�Ȃ�Agr_close���s�����邢�͎��s�ς݂ł���.

	// �X���b�h�n���h���A�C�x���g�n���h��.
	HANDLE      thread ;		// �`��X���b�h�̃n���h��.
	HANDLE   	esc_event ;
	HANDLE 		hitanykey_event ;
	HANDLE    	cw_event ;
	HANDLE 		paint_event ;

	RECT    	client_rect ;	// �N���C�A���g��`.
	ATOM 		wndclsatom ;	// RegisterClass�����l.
} gr_context;

static	gr_context gr;

//	====================================================
//	�E�B���h�E�^�C�g���������ݒ肷��.
//	====================================================
int	gr_settitle(char *name)
{
	strcpy(gr.menuname,name);
	return 0;
}

//	====================================================
//	�ĕ`��.
//	====================================================
void redraw(HDC hdc, LPRECT prc)
{
	BitBlt(hdc,
		   prc->left, prc->top,
	       prc->right - prc->left, prc->bottom - prc->top,
	       gr.hdcMem, prc->left, prc->top, SRCCOPY);
}

//	====================================================
//	(x,y)���W����s�N�Z���o�b�t�@�̃|�C���^*p�𓾂�.
//	====================================================
int *gr_point(int x,int y)
{
	if( ((u_int)x < gr.width) && ((u_int)y < gr.height) ) {
		return &gr.pixbuf[ y * gr.width + x ];
	}
	return 0;
}

//	====================================================
//	(x,y)���W�ɓ_(color)��ł�.
//	====================================================
void gr_pset(int x,int y,int color)
{
//	if(gr.hBMP==NULL) return;

	if( ((u_int)x < gr.width) && ((u_int)y < gr.height) ) {
		gr.pixbuf[ y * gr.width + x ]=color;
	}
}

#define GR_PSET(x,y,color)	gr.pixbuf[(y)*gr.width+(x)]=color

//	====================================================
//	������`�悷��.
//	====================================================
void gr_line(int x1,int y1,int x2,int y2,int c)
{
	int px,py;		/* �łׂ��_ */
	int r;			/* �덷���W�X�^ */
	int dx,dy,dir,count;

	if(x1 > x2) {SWAP(x1,x2);SWAP(y1,y2);}

	px=x1;py=y1;	/* �J�n�_ */
	dx=x2-x1;	/* �f���^ */
	dy=y2-y1;dir=1;
	if(dy<0) {dy=-dy;dir=-1;} /* ���̌X�� */

	if(dx<dy) {	/* �f���^���̕����傫���ꍇ */
		count=dy+1;
		r=dy/2;
		do {
			gr_pset(px,py,c);py+=dir;
			r+=dx;if(r>=dy) {r-=dy;px++;}
		} while(--count);
	} else {	/* �f���^���̕����傫���ꍇ */
		count=dx+1;
		r=dx/2;
		do {
			gr_pset(px,py,c);px++;
			r+=dy;if(r>=dx) {r-=dx;py+=dir;}
		} while(--count);
	}
}

//	====================================================
//	�~(cx,cy),���ar,�Fc ��`��
//	====================================================
void gr_circle( int cx,int cy,int r,int c)
{
	int  x,y;
	int  xr,yr;

	if(r==0) return;
	x=r * r;y=0;
	do {
		xr= x / r;
		yr= y / r;
		gr_pset(cx+xr,cy+yr,c);
		gr_pset(cx-xr,cy+yr,c);
		gr_pset(cx-xr,cy-yr,c);
		gr_pset(cx+xr,cy-yr,c);

		gr_pset(cx+yr,cy+xr,c);
		gr_pset(cx-yr,cy+xr,c);
		gr_pset(cx-yr,cy-xr,c);
		gr_pset(cx+yr,cy-xr,c);

		x += yr;
		y -= xr;
	}while( x>= (-y) );
}

//	====================================================
//	�~��(cx,cy),���ar,�Fc,�p�x(begin,end) ��`��
//	====================================================
void gr_circle_arc( int cx,int cy,int rx,int ry,int c,int begin,int end)
{
	float x,y,rad,t,td;
	int xr,yr;
	if(rx>ry) td = rx;
	else	  td = ry;

	td = (360/6.28) / (td * 1.2);

	for(t=begin;t<end;t=t+td) {
		rad = (t * M_PI *2 ) / 360.0;
		x =  cos(rad);
		y =  sin(rad);
		xr = x * (float)rx;
		yr = y * (float)ry;
		gr_pset(cx-xr,cy-yr,c);
		//printf("%d,%d\n",cx-xr,cy-yr);
	}
		//gr_pset(cx,cy,c);

#if	0
	int  x,y,i;
	int  xr,yr;
	if(r==0) return;
	x=r * r;y=0;
	for(i=0;i<end;i++) {
		xr= x / r;
		yr= y / r;
		gr_pset(cx+xr,cy+yr,c);
		x += yr;
		y -= xr;
	}
#endif
}

//	====================================================
//	�������C��(x1,y1)�������ׂ̃T�u���[�`��
//	====================================================
#if 0	// by senshu
static
#endif
void hline_sub(int x1,int y1,int xlen,int c)
{
	int *p = gr_point(x1,y1);
	while(xlen) {
		*p++ = c; xlen--;
	}
}

//	====================================================
//	�������C��(x1,y1)�������ׂ̃T�u���[�`��
//	====================================================
#if 0	// by senshu
static
#endif
void vline_sub(int x1,int y1,int ylen,int stride,int c)
{
	int *p = gr_point(x1,y1);
	while(ylen) {
		*p = c;	p += stride;
		ylen--;
	}
}

//	====================================================
//	�������C��(x1,y1) - (x2,y2) �������@�iy2�͖����j
//	====================================================
void gr_hline(int x1,int y1,int x2,int y2,int c)
{
	unsigned int  xlen;

	if(x2<x1) SWAP(x1,x2);
	if( ((u_int)x1 < gr.width) && ((u_int)y1 < gr.height) ) {
		xlen= x2 - x1 + 1;		/* ���̒��� */
		hline_sub(x1,y1,xlen,c);
	}
}

//	====================================================
//	�������C��(x1,y1) - (x2,y2) �������@�iy2�͖����j
//	====================================================
void gr_vline(int x1,int y1,int x2,int y2,int c)
{
	unsigned int  ylen;
//	if(gr.hBMP==NULL) return;

	if(y2<y1) SWAP(y1,y2);
	if( ((u_int)y1 < gr.height) && ((u_int)x1 < gr.width) ) {
		ylen= y2 - y1 + 1;		/* ���̒��� */
		vline_sub(x1,y1,ylen,gr.width,c);
	}
}

//	====================================================
//	�a�n�w�e�h�k�k(x1,y1) - (x2,y2),c
//	====================================================
void gr_boxfill(int x1,int y1,int x2,int y2,int c)
{
	int xlen,ylen;
	if(y2<y1) SWAP(y1,y2);
	if(x2<x1) SWAP(x1,x2);

	ylen = y2 - y1 + 1;
	xlen = x2 - x1 + 1;		/* ���̒��� */

	while(ylen) {
		hline_sub(x1,y1,xlen,c);y1++;ylen--;
	}
}


//	====================================================
//	�a�n�w(x1,y1) - (x2,y2),c
//	====================================================
void gr_box(int x1,int y1,int x2,int y2,int c)
{
	gr_hline(x1,y1,x2,y1,c);
	gr_hline(x1,y2,x2,y2,c);
	gr_vline(x1,y1,x1,y2,c);
	gr_vline(x2,y1,x2,y2,c);
}
//	====================================================
//	�`��̈��F(color)�őS�N���A.
//	====================================================
void gr_cls(int color)
{
	int i;
	int size = gr.width * gr.height;
	int *p=gr.pixbuf;
	for(i=0;i<size;i++) *p++ = color;
}
//	====================================================
//	(x,y)���W�ɐF(color)�ŕ�����(*s)��`�悷��.
//	====================================================
void gr_puts_def(int x,int y,char *s,int color)
{
	RECT r;
	r.left=x; /* �r�b�g�}�b�v�̗̈�w�� */
	r.top=y;
	r.right=gr.width;
	r.bottom=gr.height;

	/* GDI �ŕ�����`�� */
	SetTextColor(gr.hdcMem,color);
	SetBkMode(gr.hdcMem,TRANSPARENT);
	DrawText(gr.hdcMem,s,lstrlen(s),&r,DT_SINGLELINE);
}


//	====================================================
//	(x,y)���W�ɐF(color)�ŕ�����(*s)��`�悷��.
//	====================================================
void gr_puts(int x,int y,char *s,int color,int bkcolor,int fontsize)
{
	RECT r;
    HANDLE font;

	if(fontsize==0) {
		gr_puts_def(x,y,s,color);	//�ʏ�T�C�Y.
		return;
	}

	//�w��T�C�Y.
	r.left=x; /* �r�b�g�}�b�v�̗̈�w�� */
	r.top=y;
	r.right=gr.width;
	r.bottom=gr.height;

   // �t�H���g���쐬����
    font =
	   CreateFont(
	   fontsize,             // �t�H���g�̍���(�傫��)�B
	   0,                    // �t�H���g�̕��B���ʂO�B
	   0,                    // �p�x�B�O�łn�j�B
	   0,                    // �������p�x�B������O�B
	   FW_DONTCARE,          // �����̑����B
	   FALSE,                // �t�H���g���C�^���b�N�Ȃ�TRUE���w��B
	   FALSE,                // �����������Ȃ�TRUE�B
	   FALSE,                // ���������������Ȃ�TRUE�B
	   SHIFTJIS_CHARSET,     // �t�H���g�̕����Z�b�g�B���̂܂܂łn�j�B
	   OUT_DEFAULT_PRECIS,   // �o�͐��x�̐ݒ�B���̂܂܂łn�j�B
	   CLIP_DEFAULT_PRECIS,  // �N���b�s���O���x�B���̂܂܂łn�j�B
	   DRAFT_QUALITY,        // �t�H���g�̏o�͕i���B���̂܂܂łn�j�B
	   DEFAULT_PITCH,        // �t�H���g�̃s�b�`�ƃt�@�~�����w��B���̂܂܂łn�j�B
//	   "�l�r �o�S�V�b�N"     // �t�H���g�̃^�C�v�t�F�C�X���̎w��B����͌����܂�܁B
	   "Ariel"     // �t�H���g�̃^�C�v�t�F�C�X���̎w��B����͌����܂�܁B
   );

	if(font > 0) {
	    HANDLE pFont = SelectObject(gr.hdcMem,font);        // �t�H���g��ݒ�B
		/* GDI �ŕ�����`�� */
		SetTextColor(gr.hdcMem, color);
		SetBkColor(gr.hdcMem, bkcolor);
		SetBkMode(gr.hdcMem, OPAQUE);		//TRANSPARENT
		DrawText(gr.hdcMem,	s,lstrlen(s),&r,DT_SINGLELINE);

		SelectObject(gr.hdcMem,pFont);      // �t�H���g�����ɖ߂��B
		DeleteObject(font); 				/* �t�H���g�J�� */
	}
}

//	====================================================
//
//	====================================================
void gr_textout(int x,int y,char *s)
{
	RECT r;
	r.left=x; /* �r�b�g�}�b�v�̗̈�w�� */
	r.top=y;
	r.right=gr.width;
	r.bottom=gr.height;

	/* GDI �ŕ�����`�� */
	SetTextColor(gr.hdcMem,RGB(240,240,240));
	SetBkMode(gr.hdcMem,TRANSPARENT);
//	DrawText(gr.hdcMem,s,lstrlen(s),&r,DT_SINGLELINE);
	TextOut(gr.hdcMem,x,y,s,lstrlen(s));
}

//	====================================================
//	�E�B���h�E�v���V�[�W���[
//	====================================================
LRESULT CALLBACK mainwnd_proc(HWND hwnd, UINT msgid, WPARAM wparam, LPARAM lparam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	LPRECT prc;
	RECT r;

	switch(msgid) {

	case WM_MOUSEMOVE: /* �}�E�X�J�[�\���ړ� */
		gr.mouse_x=LOWORD(lparam); /* �}�E�X�J�[�\���̈ʒu�擾 */
		gr.mouse_y=HIWORD(lparam);
		return 0;

	case WM_CREATE:
		return 0;

	case WM_SIZE:
		InvalidateRect(hwnd,NULL,FALSE);
		UpdateWindow (hwnd);             // �ĕ`��
//		SetWindowPos(hwMain, NULL, 0, 0,
//			     gr.client_rect.right - gr.client_rect.left,
//			     gr.client_rect.bottom - gr.client_rect.top,
//			     SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOZORDER);
		return 0;

	case WM_SIZING:
		prc = (LPRECT) lparam;
		switch(wparam) {
		case WMSZ_TOP:
		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
		case WMSZ_BOTTOMLEFT:
		case WMSZ_LEFT:
			prc->left = prc->right - (gr.client_rect.right
						  - gr.client_rect.left);
			prc->top = prc->bottom - (gr.client_rect.bottom
						  - gr.client_rect.top);
			break;

		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMRIGHT:
		case WMSZ_RIGHT:
			prc->right = prc->left + (gr.client_rect.right
						  - gr.client_rect.left);
			prc->bottom = prc->top + (gr.client_rect.bottom
						  - gr.client_rect.top);
			break;
		}
		return 0;

	case WM_PAINT:
		{ int x,y;
			GetClientRect(hwnd,&r); /* �N���C�A���g�̈�擾 */
			x=(r.right -gr.width) /2; /* �r�b�g�}�b�v�̕\���ʒu�v�Z */
			y=(r.bottom-gr.height)/2;

			hdc=BeginPaint(gr.hwMain,&ps);

			BitBlt(hdc,x,y,gr.width,gr.height,gr.hdcMem,0,0,SRCCOPY);

			EndPaint(gr.hwMain,&ps);
		}
		SetEvent(gr.paint_event);
//	BringWindowToTop(gr.hwMain);
		return 0;

	case WM_KEYDOWN:
		if(wparam == VK_ESCAPE) {
			gr.esc_flag = 1;
			gr.end_flag = 1;
			SetEvent(gr.esc_event);
		}
		if(lparam & (1 << 30))
			return 0;

		if(wparam == VK_F5) {
			hdc = GetDC(gr.hwMain);
			redraw(hdc, &gr.client_rect);
			ReleaseDC(gr.hwMain, hdc);
			return 0;
		}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		SetEvent(gr.hitanykey_event);
		return 0;

	case WM_DESTROY:
		DeleteDC(gr.hdcMem); /* �f�o�C�X�R���e�L�X�g�J�� */
		gr.hdcMem = NULL;
#if	0
		DeleteObject(gr.hBMP); /* �r�b�g�}�b�v�J�� */
		gr.hBMP   = NULL;

		if(	gr.lpBuf ) {
			GlobalFree(gr.lpBuf); /* ����������� */
			gr.lpBuf = NULL;
		}
#endif
		PostQuitMessage(0);
		gr.end_flag = 1;
		return 0;
	}

	return DefWindowProc(hwnd, msgid, wparam, lparam);
}

//	====================================================
//	�E�B���h�E����
//	====================================================
void init_window(void)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = mainwnd_proc;
	wcex.cbClsExtra = wcex.cbWndExtra = 0;
	wcex.hInstance = gr.instance = GetModuleHandle(NULL);
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "graphics";
	wcex.hIconSm = wcex.hIcon;
	gr.wndclsatom = RegisterClassEx(&wcex);
	if(gr.wndclsatom == 0) {
		fprintf(stderr, "Can't register class.\n");
		exit(EXIT_FAILURE);
	}

	AdjustWindowRectEx(
		&gr.client_rect,
		WS_OVERLAPPEDWINDOW,
		FALSE,
		WS_EX_OVERLAPPEDWINDOW);

	gr.hwMain = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		"graphics",
		gr.menuname,
		WS_OVERLAPPEDWINDOW ,
		CW_USEDEFAULT, CW_USEDEFAULT,
		gr.client_rect.right  - gr.client_rect.left,
		gr.client_rect.bottom - gr.client_rect.top,
		NULL, NULL, gr.instance, NULL);

	if(gr.hwMain == NULL) {
		fprintf(stderr, "Can't create window.\n");
		UnregisterClass("graphics", gr.instance);
		exit(EXIT_FAILURE);
	}


	ShowWindow(gr.hwMain, SW_NORMAL);
	UpdateWindow(gr.hwMain);

//	SetActiveWindow(gr.hwMain);
//	BringWindowToTop(gr.hwMain);
SetForegroundWindow(gr.hwMain);
}

//	====================================================
//	�c�h�a�r�b�g�}�b�v����
//	====================================================
void init_bitmap(int bpp)
{
	LPBITMAPINFO lpDIB;
	LPBYTE 		 lpBMP;
	HDC 		 hdcWin;
	gr.lpBuf = (LPBYTE)GlobalAlloc(GPTR,sizeof(BITMAPINFO));
	lpDIB=(LPBITMAPINFO)gr.lpBuf;
	lpDIB->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	lpDIB->bmiHeader.biWidth =  gr.width;
	lpDIB->bmiHeader.biHeight= -gr.height;	//�オy=0��bitmap
	lpDIB->bmiHeader.biPlanes=1;
	lpDIB->bmiHeader.biBitCount=bpp;	//16;
	lpDIB->bmiHeader.biCompression=BI_RGB;
	lpDIB->bmiHeader.biSizeImage=0;
	lpDIB->bmiHeader.biXPelsPerMeter=0;
	lpDIB->bmiHeader.biYPelsPerMeter=0;
	lpDIB->bmiHeader.biClrUsed=0;
	lpDIB->bmiHeader.biClrImportant=0;

	hdcWin=GetDC(gr.hwMain); /* �E�C���h�E��DC ���擾 */

	/* DIB �ƃE�C���h�E��DC ����DIBSection ���쐬 */
#ifdef __BORLANDC__
	gr.hBMP=(HBITMAP)CreateDIBSection(hdcWin, lpDIB, DIB_RGB_COLORS, (void **)&lpBMP,NULL,0);
#else
	gr.hBMP=(HBITMAP)CreateDIBSection(hdcWin, lpDIB, DIB_RGB_COLORS, (void *)&lpBMP,NULL,0);
#endif

	gr.hdcMem=CreateCompatibleDC(hdcWin); /* ������DC ���쐬 */

	SelectObject(gr.hdcMem,gr.hBMP); /* ������DC �Ƀr�b�g�}�b�v��I�� */

	ReleaseDC(gr.hwMain,hdcWin);

	gr.pixbuf = (int*)lpBMP;
}

//	====================================================
//	�E�B���h�E���C�������p�̃X���b�h.
//	====================================================
DWORD WINAPI xmain(LPVOID param)
{
	MSG msg;

	init_window();

	SetEvent(gr.cw_event);

	gr.destroy_flag = 0;
	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if(gr.destroy_flag) {
			DestroyWindow(gr.hwMain);
			gr.hwMain=NULL;
		}
	}

	gr.end_flag = 1;
	gr.esc_flag = 0;
//	DeleteObject(bitmap);

	return msg.wParam;
}

//	====================================================
//	��ʃt���b�v.
//	====================================================
int	gr_flip(int flag)
{
	ResetEvent(gr.paint_event);
	InvalidateRect(gr.hwMain,NULL,FALSE);
	WaitForSingleObject(gr.paint_event, INFINITE);

	return gr.end_flag;
}

//	====================================================
//	�ĕ`��𑣂��Ɠ����ɁA�I���`�F�b�N�̒l��Ԃ�.
//	====================================================
int	gr_break(void)
{
	if( gr.hwMain != NULL) {
		InvalidateRect(gr.hwMain,NULL,FALSE);
	}
	return gr.end_flag;
}

//	====================================================
//
//	====================================================
int hitanykey(void)
{
	char fnbuf[MAX_PATH];

	gr_flip(0);

	GetWindowsDirectory(fnbuf, MAX_PATH);
	ResetEvent(gr.hitanykey_event);
	WaitForSingleObject(gr.hitanykey_event, INFINITE);

	return gr.esc_flag;
}


//	====================================================
//	�O���t�B�b�N�I������.
//	====================================================
void gr_close(void)
{
	DWORD exitcode;

	if(gr.close_flag) return;

	GetExitCodeThread(gr.thread, &exitcode);
	if(exitcode == STILL_ACTIVE) {
		gr.destroy_flag = 1;
		while(exitcode == STILL_ACTIVE) {
			GetExitCodeThread(gr.thread, &exitcode);
		}
	}
	if(	gr.wndclsatom != 0 ) {
		UnregisterClass("graphics", gr.instance);
	}
	CloseHandle(gr.esc_event);
	CloseHandle(gr.hitanykey_event);
	CloseHandle(gr.cw_event);
	CloseHandle(gr.paint_event);

	if(	gr.hBMP ) {
		DeleteObject(gr.hBMP); /* �r�b�g�}�b�v�J�� */
		gr.hBMP   = NULL;
	}
	if( gr.lpBuf ) {
		GlobalFree(gr.lpBuf);
		gr.lpBuf = NULL;
	}
	gr.close_flag = 1;
}


//	====================================================
//	�O���t�B�b�N������.
//	====================================================
void gr_init(int width,int height,int bpp,int color)
{
	//static
	int first = 1;

	width = (width + 3) & (-4);	//�S�̔{���ɂ���.

	memset(&gr,0,sizeof(gr));
	gr.width  = width;
	gr.height = height;
	gr_settitle("picmon");

	gr.client_rect.left  = 32;
	gr.client_rect.top   = 32;

	gr.client_rect.right  = gr.client_rect.left + width;
	gr.client_rect.bottom = gr.client_rect.top  + height;

	gr.end_flag = 0;

	if(first) {
		DWORD thid;

		atexit(gr_close);
		first = 0;
		gr.close_flag = 0;

		gr.esc_event       = CreateEvent(NULL, FALSE, TRUE, NULL);
		gr.hitanykey_event = CreateEvent(NULL, FALSE, TRUE, NULL);
		gr.cw_event        = CreateEvent(NULL, FALSE, FALSE, NULL);
		gr.paint_event     = CreateEvent(NULL, FALSE, FALSE, NULL);
		gr.thread = CreateThread(NULL, 0, xmain, NULL, 0, &thid);
		if(gr.thread == NULL) {
			fprintf(stderr, "Can't create thread.\n");
			exit(EXIT_FAILURE);
		}
		WaitForSingleObject(gr.cw_event, INFINITE);
	}
	init_bitmap(bpp);
}
#else
#endif
