//	====================================================
//	�b����:�ȈՃO���t�B�b�N���C�u����(Win32��)
//	====================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef	_LINUX_
#include <windows.h>
#endif

//	====================================================
//	��`.
//	====================================================
//#define	RGB_(r,g,b) (((b)>>3)|(((g)>>3)<<5)|(((r)>>3)<<10))	// 16bpp�̂Ƃ��̐F.

//	====================================================
//	�C���^�[�t�F�[�X.
//	====================================================
void gr_init(int width,int height,int bpp,int color);	//������.
void gr_exit(int rc);									//�I��.

void gr_cls(int color);									//��ʃN���A.
void gr_pset(int x,int y,int color);					//�h�b�g�ł�.
int *gr_point(int x,int y);
void gr_line(int x0,int y0,int x1,int y1,int color);	//������.
void gr_hline(int x0,int y0,int x1,int y1,int color);	//������.
void gr_vline(int x0,int y0,int x1,int y1,int color);	//������.
void gr_box(int x0,int y0,int width,int height,int color);		//��(�g�̂�).
void gr_boxfill(int x0,int y0,int width,int height,int color);	//��(�����h��Ԃ�).
void gr_circle( int cx,int cy,int r,int c);
void gr_circle_arc( int cx,int cy,int rx,int ry,int c,int begin,int end);


void gr_puts(int x,int y,char *s,int color,int bkcolor,int size);	// ������`��.

int gr_flip(int flag);									//�`�抮������.
void gr_close(void);									//��close
int gr_break(void);										//�I���`�F�b�N.
int hitanykey(void);									//�L�[���̓`�F�b�N.

#if	0	//������.

#define	FLIP_NOWAIT	0
#define	FLIP_WAIT	1

typedef	struct {
	int	id;
	int width;
	int height;
	//
	
	//
} gr_bitmap;

gr_bitmap *gr_loadbmp(char *filename);					//bmp�t�@�C���̓ǂݍ���.
void gr_putbmp(int x,int y,gr_bitmap *bitmap);			//bmp�t�@�C���̕\��.
void gr_putsprite(int x,int y,gr_bitmap *bitmap);		//���������t���̕\��.
#endif

//	====================================================
