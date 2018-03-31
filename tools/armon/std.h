/*********************************************************
 *	�b����@�ȗ��w�b�_�[  Borland-C version. rev.2
 *********************************************************
 */
#include <stdio.h>
/*
 *	�����Ȃ��̌^���̏ȗ��`
 *	typedef�̏ꍇ�͔z���؂�Ƃ��Ƀ��[�h�A���C�����g�����̂�
 *	������#define�`�ɂ��Ă���܂��B
 */

#define Uchar unsigned char
#define Uint  unsigned int
#define Ulong unsigned long

/*
 *	�I�v�V����������`�F�b�N
 */
char  *opt[128];	/* �I�v�V�����������w�肳��Ă����炻�̕�����
			   ������������i�[�A�w��Ȃ����NULL	*/

#define Getopt(argc,argv)  \
 {int i;for(i=0;i<128;i++) opt[i]=NULL; \
   while( ( argc>1 )&&( *argv[1]=='-') ) \
    { opt[ argv[1][1] & 0x7f ] = &argv[1][2] ; argc--;argv++; } \
 }

#define IsOpt(c) ((opt[ c & 0x7f ])!=NULL)
#define   Opt(c)   opt[ c & 0x7f ]


/*
 *	�Y�{����������
 */
#define Main main(int argc,char **argv){ Getopt(argc,argv);
#define End  exit(0);}

#define Seek(fp,ptr) fseek(fp,ptr,0)

/*
 *	�O���[�o���ϐ�
 */
FILE  *ifp;
FILE  *ofp;
FILE  *ifp2;
FILE  *ofp2;

/*
 *	�t���������i"������"�j
 */
#define Usage(string) usage(){printf(string);exit(1);}

/*
 *	�ȗ��^�t�@�C���A�N�Z�X
 */
#define Ropen(name) {ifp=fopen(name,"rb");if(ifp==NULL) \
{ printf("Fatal: can't open file:%s\n",name);exit(1);}  \
}

#define Wopen(name) {ofp=fopen(name,"wb");if(ofp==NULL) \
{ printf("Fatal: can't create file:%s\n",name);exit(1);}  \
}

#define Ropen2(name) {ifp2=fopen(name,"rb");if(ifp2==NULL) \
{ printf("Fatal: can't open file:%s\n",name);exit(1);}  \
}

#define Wopen2(name) {ofp2=fopen(name,"wb");if(ofp2==NULL) \
{ printf("Fatal: can't create file:%s\n",name);exit(1);}  \
}

#define Read(buf,siz)   fread (buf,1,siz,ifp)
#define Write(buf,siz)  fwrite(buf,1,siz,ofp)
#define Read2(buf,siz)  fread (buf,1,siz,ifp2)
#define Write2(buf,siz) fwrite(buf,1,siz,ofp2)

#define Rclose()  fclose(ifp)
#define Wclose()  fclose(ofp)
#define Rclose2()  fclose(ifp2)
#define Wclose2()  fclose(ofp2)

