#include <stdio.h>
#include <graphics.h>
#include "tools.h"
#include "time.h"
#include <math.h>
#include "vector2.h"

//������Ч
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
#define WIN_WIDTH 900
#define WIN_HEIGHT 600
#define ZOM_MAX 10
/*
1.��������Ŀ
2.�����ز�
5.ʵ�ֹ������е�ֲ�￨��
*/
enum {WAN_DOU,XIANG_RI_KUI,PLANTS_COUNTS};

IMAGE imBg;
IMAGE imBar;
IMAGE imgCards[PLANTS_COUNTS];
IMAGE* imgPLANTS[PLANTS_COUNTS][20];

enum {GOING,WIN,FAIL};
int killCount;
int zomCount;
int gameStatus;


int curX, curY;//��ǰѡ�е�ֲ����ƶ������е�λ��
int curPLANTS;//0:û��ѡ��ֲ�� 1:ѡ���˵�һ��ֲ��\
//ֲ��ṹ��
struct Plants
{
	int type;	//0:û��ֲ��,1:��ʾ��һ��
	int frameIndex;//����֡���
	bool catched;//�Ƿ񱻽�ʬ����
	int deadTimer; //������ʱ��
	int timer;
	int x, y;//ֲ������
	int shootTime;
};

struct Zom
{
	int x, y;
	int frameIndex;
	bool used;
	int speed;
	int row;
	int blood;
	bool dead;
	bool eating;//���ڳ�ֲ��
};
struct Zom zoms[10];
IMAGE imgZOM[22];
IMAGE imgZomDead[20];
IMAGE imgZomEat[21];
//�����ӵ�
struct bullet
{
	int x, y;
	int row;
	bool used;
	int speed;
	//�Ƿ�����ը
	bool blast;
	int frameIndex;//֡���
};
struct bullet bullets[30];
IMAGE imgBulletNormal;
IMAGE imgBallBlast[4];
IMAGE imgZomStand[11];
struct Plants map[3][9];
enum {SUNSHINE_DOWN,SUNSHINE_GROUND,SUNSHINE_COLLECT,SUNSHINE_PRODUCT};

struct sunshineBall
{
	int x, y;
	int frameIndex;//��ǰ��ʾͼƬ֡���
	int destY;//Ʈ���Ŀ��λ�õ�y����
	bool used;//�Ƿ���ʹ��
	int timer;//��ʱ��
	//ƫ����
	float xoff;
	float yoff;

	float t;//���������ߵ�ʱ���0..1
	vector2 p1, p2, p3, p4;
	vector2 pCur;//��ǰʱ���������λ��
	float speed;
	int status;
};

//��˿������ػ�˼��.....
struct sunshineBall balls[10];
IMAGE imgSunShineBall[29];
int SunShine;


//�ж��ļ��Ƿ���ں���
bool fileExist(const char* name) {
	FILE* fp = fopen(name, "r");
	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}

void gameInit() {
	//��Ϸ��ʼ��,������Ϸ����ͼƬ
	//�ַ�����Ϊ���ֽ��ַ���
	loadimage(&imBg,"res/bg.jpg");
	loadimage(&imBar, "res/bar5.png");
	//����һ���ڴ�
	memset(imgPLANTS, 0, sizeof(imgPLANTS));
	memset(map, 0, sizeof(map));
	
	killCount = 0;
	zomCount = 0;
	gameStatus = GOING;


	//��ʼ��ֲ�￨��
	char name[64];
	for (int i = 0; i < PLANTS_COUNTS; i++)
	{
		sprintf_s(name,sizeof(name),"res/Cards/card_%d.png",i+1);
		loadimage(&imgCards[i], name);

		for (int j = 0; j < 20; j++)
		{
		//��ʼ��ֲ��
		//����Ҫע�⣬��Ҫ������������error����ᱨ��
		sprintf_s(name, sizeof(name),"res/zhiwu/%d/%d.png",i,j + 1);
		//���ж��ļ��Ƿ����
		if (fileExist(name)) {
			//�ȷ����ڴ棬c++���䷽ʽ��C������malloc

			imgPLANTS[i][j] = new IMAGE;
			loadimage(imgPLANTS[i][j],name);
		}
		}
		for (int i = 0; i < 21; i++)
		{ 
			sprintf_s(name, "res/zm_eat/%d.png", i + 1);
			loadimage(&imgZomEat[i], name);
		}

	}

	//������Ϸ����
	curPLANTS = 0;
	SunShine = 50;
	//�������ballΪ0
	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunShineBall[i], name);
	}
	srand(time(NULL));

	initgraph(WIN_WIDTH, WIN_HEIGHT,1);
	//��ʾ����ֵ
	//����ı�
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	//΢���ź�
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;//�����
	settextstyle(&f);
	//���ñ�����ɫ͸��
	setbkmode(TRANSPARENT);
	//����������ɫ
	setcolor(BLACK);

	//��ʼ����ʬ����
	memset(zoms, 0, sizeof(zoms));
	for (int i = 0; i < 22; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZOM[i], name);
	}

	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));
	//��ʼ���㶹�ӵ���֡ͼƬ����
	loadimage(&imgBallBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++)
	{
		float k = (i + 1) * 0.2;
		loadimage(&imgBallBlast[i], "res/bullets/bullet_blast.png",
		imgBallBlast[3].getwidth()* k,
		imgBallBlast[3].getheight()* k, true);
	}
	for (int i = 0; i < 20; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZomDead[i], name);
	}

	for (int i = 0; i < 11; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZomStand[i], name);
	}
}
//��Ⱦ��ʬ
void drawZom() {
	int zomCount = sizeof(zoms) / sizeof(zoms[0]);
	for (int i = 0; i < zomCount; i++)
	{	
		if (zoms[i].used)
		{
			IMAGE* img = NULL;
			//�жϽ�ʬ״ִ̬����Ⱦ����
			if (zoms[i].dead) img = imgZomDead;
			else if (zoms[i].eating) img = imgZomEat;
			else img = imgZOM;
			img += zoms[i].frameIndex;
			putimagePNG(zoms[i].x, zoms[i].y - img->getheight(), img);
		}
	}
}

//void drawBullet() {
//
//	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
//	int i = 0;
//	for (i = 0; i < bulletMax; i++);
//		if (bullets[i].used)
//		{
//			putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
//		}
//}
//��������
void drawSunShines() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++)
	{
		//if (balls[i].used || balls[i].xoff) {
		if(balls[i].used){
		IMAGE* img = &imgSunShineBall[balls[i].frameIndex];
		//putimagePNG(balls[i].x, balls[i].y, img);
		putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}

	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", SunShine);
	outtextxy(280, 70, scoreText);
}

void drawCards() {
	for (int i = 0; i < PLANTS_COUNTS; i++)
	{
		int x = 338 + i * 65;
		int y = 6;
		putimagePNG(x, y, &imgCards[i]);
	}
}
void drawPlants() {
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0)
			{
				int x = 256 + j * 81;
				int y = 170 + i * 95 + 14;
				int PlantsType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				//putimagePNG(x, y, imgPLANTS[PlantsType][index]);
				putimagePNG(map[i][j].x, map[i][j].y, imgPLANTS[PlantsType][index]);
			}
		}
	}

	//�϶������е�ֲ��
	if (curPLANTS > 0) {
		IMAGE* img = imgPLANTS[curPLANTS - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);
	}
}

void drawBullets() {
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int i = 0;
	for (i = 0; i < bulletMax; i++) {
		//�����ж��Ƿ�ʹ��
		if (bullets[i].used)
		{	//���״̬��blast����Ⱦ��ը
			if (bullets[i].blast)
			{
				IMAGE* img = &imgBallBlast[bullets[i].frameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
				FlushBatchDraw();
			}
			else//������Ⱦ���
			{
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}
		}
	}
}
//��ȾͼƬ
void updateWindow() {
	//˫����
	BeginBatchDraw();//��ʼ����
	putimage(0, 0, &imBg);
	putimagePNG(250, 0, &imBar);
	drawCards();
	//��ֲֲ��
	drawPlants();
	//��Ⱦ����
	drawSunShines();
	drawZom();
	//drawBullet();
	drawBullets();
	EndBatchDraw();
}

//��ȡ����
void collectSunShine(ExMessage* msg) {
	int sunshinecount = sizeof(balls) / sizeof(balls[0]);
	//��ȡ���������
	int w = imgSunShineBall[0].getwidth();
	int h = imgSunShineBall[0].getheight();

	for (int i = 0; i < sunshinecount; i++)
	{
		if (balls[i].used)
		{
			//int x = balls[i].x;
			//int y = balls[i].y;
			int x = balls[i].pCur.x;
			int y = balls[i].pCur.y;

			if (msg->x > x && msg-> x < x+w &&
				msg->y > y && msg-> y < y+h) {
				//balls[i].used = false;
				balls[i].status = SUNSHINE_COLLECT;
				//SunShine += 25;
				//�������ֺ���
				//mciSendString("play res/sunshine.mp3",0,0,0);
				PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);
				//�����������ƫ����
				//float destY = 0;
				//float destX = 262;
				//float angle = atan((y - destY) / (x - destX));
				//balls[i].xoff = 4 * cos(angle);
				//balls[i].yoff = 4 * sin(angle);
				//���Ʊ�������
				balls[i].p1 = balls[i].pCur;
				balls[i].p4 = vector2(262, 0);
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);
				float off = 8;

				balls[i].speed = 1.0 / (distance / off);
				break;

			}
		}
	}
}


void userClick() {
	//�ж����״̬����
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) {
		//�����������ʱ��,���ķ�Χ
		if (msg.message == WM_LBUTTONDOWN) {
			if (msg.x > 338 && msg.x < 338 + 65 * PLANTS_COUNTS && msg.y < 96) {
				int index = (msg.x - 338) / 65;
				status = 1;
				curPLANTS = index + 1;
			}
			else
			{
				collectSunShine(&msg);
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1) {
			//�����Ⱦλ��
			curX = msg.x;
			curY = msg.y;
			//ȷ����Ⱦ��һ��ֲ��

		}
		else if (msg.message == WM_LBUTTONUP) {
			if (msg.x > 256 && msg.y > 179 && msg.y < 489 )
			{
				int row = (msg.y - 260) / 81.6;
				int col = (msg.x - 256) / 81;

				if (map[row][col].type == 0)
				{
					map[row][col].type = curPLANTS;
					map[row][col].frameIndex = 0;
					map[row][col].shootTime = 0;
					//����ֲ��λ��
					map[row][col].x = 256 + col * 81;
					map[row][col].y = 179 + row * 102 + 14;
				}

			}
			curPLANTS = 0;
			status = 0;
		}
	}

}

void createSunShine() {

	static int count = 0;
	static int fre = 400;
	count++;
	if (count >= fre)
	{
		fre = 200 + rand() % 200;
		count = 0;

		//���������ȡһ������ʹ�õ�
		int ballMax = sizeof(balls) / sizeof(balls[0]);
		int i;
		for (i = 0; i < ballMax && balls[i].used; i++);
		if (i >= ballMax) return;

		balls[i].used = true;
		balls[i].frameIndex = 0;
		//balls[i].x = 260 + rand() % 900 - 260;
		//balls[i].y = 60;
		//balls[i].destY = 200 + rand() % 4 * 90;
		balls[i].timer = 0;
		//balls[i].xoff = 0;
		//balls[i].yoff = 0;
		//�������߷�����������
		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(260 + rand() % 900 - 260, 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		int off = 2;
		float distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.0 / (distance / off);

	}
		//���տ���������
		int ballMax = sizeof(balls) / sizeof(balls[0]);
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 9;j++) {
				if (map[i][j].type == XIANG_RI_KUI +1)
				{
					map[i][j].timer++;
					if (map[i][j].timer > 200)
					{
						map[i][j].timer = 0; 
						int k;
						for (k = 0; k < ballMax && balls[k].used; k++);
						if (k >= ballMax) return;

						balls[k].used = true;
						balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
						int w = (100 + rand() % 50) * (rand() % 2 ? 1 : -1);
						balls[k].p4 = vector2(map[i][j].x + w,
							map[i][j].y + imgPLANTS[XIANG_RI_KUI][0]->getheight()- 
							imgSunShineBall[0].getheight());
						balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
						balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 100);
						balls[k].status = SUNSHINE_PRODUCT;
						balls[k].speed = 0.05;
						balls[k].t = 0;
					}
				}
			}
		}
	}


void updateSunShine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++)
	{
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN)
			{
				struct sunshineBall* sun = &balls[i];
				sun -> t += sun->speed;
				sun->pCur =sun -> p1 + sun->t * (sun->p4 - sun->p1);
				if (sun ->t >=1)
				{
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer >100)
				{
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t >1)
				{
					sun->used = false;
					SunShine += 25;
				}

			}
			else if(balls[i].status == SUNSHINE_PRODUCT)
			{
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun ->t,sun->p1,sun->p2,sun->p3,sun->p4);
				if (sun->t >1)
				{
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}

		}
	}
}
//������ʬ
void createZom() {
	if (zomCount >= ZOM_MAX)
	{
		return;
	}
	//����Ƶ��
	static int zomFre = 200;
	static int count = 0;

	count++;
	if (count > zomFre)
	{
		count = 0;
		zomFre = 300 + rand() % 200 ;

		int i;
		int zomMax = sizeof(zoms) / sizeof(zoms[0]);
		//ע������for��������ס�����if
		for (i = 0; i < zomMax && zoms[i].used; i++);
			if (i < zomMax)
			{
				memset(&zoms[i], 0, sizeof(zoms[i]));
				zoms[i].used = true;
				zoms[i].x = WIN_WIDTH;
				zoms[i].row = rand() % 3;
				zoms[i].y = 172 + (1 + zoms[i].row) * 100;
				zoms[i].speed = 1;
				//��ʬ��ײ���
				zoms[i].blood = 100;
				zoms[i].dead = false;
				zomCount++;
			}
	}
	
}

void updateZom() {
	//��ý�ʬ����
	int zomMax = sizeof(zoms) / sizeof(zoms[0]);
	static int count = 0;
	count++;
	if (count > 4)
	{
		count = 0;
		//����λ��
		for (int i = 0; i < zomMax; i++)
		{
			if (zoms[i].used) {

				zoms[i].x -= zoms[i].speed;
				if (zoms[i].x < 160)
				{
					//printf("game over \n");
					//MessageBox(NULL, "over", "over", 0);//���Ż�
					//exit(0);//��Ϸ�����Ż�
					gameStatus = FAIL;
				}
			}
		}
	}
	
	//�ý�ʬ������
	static int count2 = 0;
	count2++;
	if (count2 > 4)
	{
		count2 = 0;

		for (int i = 0; i < zomMax; i++)
		{	//��ʬ�������Ŷ���
			if (zoms[i].used) {
				if (zoms[i].dead)
				{
					zoms[i].frameIndex++;
					if (zoms[i].frameIndex >= 20)
					{
						zoms[i].used = false;
						killCount++;
						if (killCount == ZOM_MAX)
						{
							gameStatus = WIN;
						}
					}
				}
				//��ʬ��ֲ�ﲥ��
				else if (zoms[i].eating)
				{
					zoms[i].frameIndex = (zoms[i].frameIndex + 1) % 21;
				}//��ʬ���߲���
				else {
					zoms[i].frameIndex = (zoms[i].frameIndex + 1) % 22;
				}
			}
		}
	}
	
}

void shoot(){
	//static int count = 0;
	//if (++count < 4) return;
	//count = 0;
	//�ж���һ�����޽�ʬ
	int lines[3] = { 0 };
	int zomCount = sizeof(zoms) / sizeof(zoms[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int dangerX = WIN_WIDTH - imgZOM[0].getwidth() + 50;
	for (int i = 0; i < zomCount; i++)
	{
		if (zoms[i].used && zoms[i].x < dangerX )
		{
			lines[zoms[i].row] = 1;
		}
	}

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9;j++) {
			if (map[i][j].type == WAN_DOU +1 && lines[i]) {
//			if (map[i][j].type == 1 && lines[i]) {

				//static int count = 0;
				//count++;
				map[i][j].shootTime++;
				if (map[i][j].shootTime > 20)
				//if (count >20 )
				{
					map[i][j].shootTime = 0;
					//count = 0;
					//ѭ�������ӵ����
					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);
					if (k < bulletMax)
					{
						bullets[k].used = true;
						//ע��������i�����������k�ͻ�������
						bullets[k].row = i;
						bullets[k].speed = 6 ;
						bullets[k].blast = false;
						bullets[k].frameIndex = 0;
						//int x = 256 + j * 81;
						//int y = 170 + i * 95;
						int PlantsX = 256 + j * 81;
						int PlantsY = 170 + i * 95+14;
						//�ӵ�ƫ����
						bullets[k].x = PlantsX + imgPLANTS[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = PlantsY + 18;
					}
				}
			}
		}
	}
	
}

void updateBullet() {
	//static int count = 0;
	//if (++count < 8) return;
	//count = 0;
	int countMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < countMax; i++)
	{
		if (bullets[i].used)
		{	
			bullets[i].x += bullets[i].speed;
			if(bullets[i].x > WIN_WIDTH){
			 bullets[i].used = false;
			}
			//��ʵ���ӵ�����ײ���
			if (bullets[i].blast)
			{
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex >= 4)
				{
					bullets[i].used = false;
				}
			}
		}
	}

}
//ʵ���ӵ���ײ���

void checkBullet2Zom() {
	int bCount = sizeof(bullets) / sizeof(bullets[0]);
	int zCount = sizeof(zoms) / sizeof(zoms[0]);
	for (int i = 0; i < bCount; i++)
	{
		if (bullets[i].used == false || bullets[i].blast)continue;
		for (int k = 0; k < zCount; k++)
		{
			if (zoms[k].used == false) continue;
			int x1 = zoms[k].x + 10;
			int x2 = zoms[k].x + 110;
			int x = bullets[i].x;

			if (zoms[k].dead == false &&
				bullets[i].row == zoms[k].row && x > x1 && x < x2)
			{
				zoms[k].blood -= 20;
				bullets[i].blast = true;
				bullets[i].speed = 0;
				if (zoms[k].blood <= 0)
				{
					zoms[k].dead = true;
					zoms[k].speed = 0;
					zoms[k].frameIndex = 0;
				}

				break;
			}
		}
	}


}

void checkZom2Plants() {
	int zCount = sizeof(zoms) / sizeof(zoms[0]);

	for (int i = 0; i < zCount; i++)
	{	
		if (zoms[i].dead)continue;
		int row = zoms[i].row;
		for (int k = 0; k < 9; k++)
		{
			if (map[row][k].type == 0) continue;
			int PlantX = 256 + k * 81;
			int x1 = PlantX + 10;
			int x2 = PlantX + 60;
			int x3 = zoms[i].x + 80;
			if (x3 > x1 && x3 < x2 )
			{
				if (map[row][k].catched) {
					//zoms[i].frameIndex++;
					map[row][k].deadTimer++;
					//if (zoms[i].frameIndex > 100)
					if(map[row][k].deadTimer > 100)
					{
						map[row][k].deadTimer = 0;
						map[row][k].type = 0;
						zoms[i].eating = false;
						zoms[i].frameIndex = 0;
						zoms[i].speed = 1;
					}

				}
				else
				{
					map[row][k].catched = true;
					map[row][k].deadTimer = 0;
					zoms[i].eating = true;
					zoms[i].speed = 0;
					zoms[i].frameIndex = 0;
				}
			}
		}
	}

}

void collisionCheck() {
	checkBullet2Zom();//�ӵ��Խ�ʬ��ײ���
	checkZom2Plants();//��ʬ��ֲ�����ײ���
}

void updatePlants() {
	static int count = 0;
	if (++count < 2) return;
	count = 0;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0)
			{
				map[i][j].frameIndex++;
				int PlantsType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;

				if (imgPLANTS[PlantsType][index] == NULL) {
					map[i][j].frameIndex = 0;
				}
			}

		}
	}
}

//ѭ����Ⱦֲ���ֲ�ﶯ����
void updateGame() {
	updatePlants();
	createSunShine();
	//��������״̬
	updateSunShine();
	//������ʬ
	createZom();
	//���½�ʬ״̬
	updateZom();
	
	//�����ӵ�
	shoot();
	updateBullet();
	collisionCheck();//��ײ���
}
//������ʼ�˵�
void startUI() {
	IMAGE imgBgA,imgMenu1,imgMenu2;
	loadimage(&imgBgA, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");

	int flag = 0;

	while (1)
	{
		BeginBatchDraw();
		putimagePNG(0, 0, &imgBgA);
		putimagePNG(474, 75,flag ? &imgMenu2: &imgMenu1);

		ExMessage msg;
		if (peekmessage(&msg))
		{
			if (msg.message == WM_LBUTTONDOWN && 
				msg.x >474 && msg.x < 474 + 300 && 
				msg.y < 175 && msg.y < 75+140) {
				flag = 1;

			}
			else if (msg.message == WM_LBUTTONUP && flag == 1)
			{
				EndBatchDraw();
				break;
			}
		}
		EndBatchDraw();
	}
	
}


void viewScence() {
	int xMin = WIN_WIDTH - imBg.getwidth();//900-1400
	vector2 points[9] = {
		{650,80},{630,160},{730,170},{630,200},{615,270},
		{665,370},{705,340},{805,280},{790,340}
	};
	int index[9];
	for (int i = 0; i < 9; i++)
	{
		index[i] = rand() % 11;
	}
	int count = 0;

	for (int x = 0; x >= -400; x--)
	{
		BeginBatchDraw();
		putimage(x, 0, &imBg);

		count++;
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x - xMin + x,
				points[k].y,&imgZomStand[index[k]]);
			if (count >= 10)
			{
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10)count = 0;
		EndBatchDraw();
		Sleep(5);
	}
	for (int i = 0; i < 100; i++)
	{
		BeginBatchDraw();
		putimage(xMin, 0, &imBg);
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x, points[k].y, &imgZomStand[index[k]]);
			index[k] = (index[k] + 1) % 11;
		}
		EndBatchDraw();
		Sleep(50);
	}
	for (int x = xMin; x <= 0 ; x+=2)
	{
		BeginBatchDraw();
		putimage(x, 0, &imBg);
		count++;
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x-xMin+x, points[k].y, &imgZomStand[index[k]]);
			if (count >= 10)
			{
				index[k] = (index[k] + 1) % 11;
			}if (count >= 10) count = 0;
		}
		EndBatchDraw();
		Sleep(5);
	}

}
void barsDown() {
	int height = imBar.getheight();
	for (int y = -height; y <=0; y++)
	{
		BeginBatchDraw();
		putimage(0, 0, &imBg);
		putimagePNG(250, y, &imBar);
		for (int i = 0; i < PLANTS_COUNTS; i++)
		{
			int x = 338 + i * 65;
			putimagePNG(x, 6+y, &imgCards[i]);
		}
		EndBatchDraw();
		Sleep(5);
	}
}

bool checkOver() {
	int ret = false;
	if (gameStatus == WIN)
	{
		Sleep(100);
		loadimage(0, "res/win2.png");
		ret = true;
		
	}
	else if (gameStatus == FAIL) {
		Sleep(100);
		loadimage(0, "res/fail2.png");
		ret = true;
	}
	return ret;
}

int main(void) {
	gameInit();
	startUI();
	viewScence();
	barsDown();
	int timer = 0;
	int flag = true;
	updateWindow();
	while (1) {
		userClick();
		//��ȡ��ʱʱ�亯��
		//���û������꿪ʼ��ʱ
		timer += getDelay();
		if (timer > 20)
		{
			flag = true;
			timer = 0;
		}

		if (flag)
		{
			flag = false;
			updateWindow();
			updateGame();
			//checkOver();
			if (checkOver()) break;
		}

	}
	system("pause");
	return 0;
}

