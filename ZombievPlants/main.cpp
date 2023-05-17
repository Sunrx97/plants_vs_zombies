#include <stdio.h>
#include <graphics.h>
#include "tools.h"
#include "time.h"
#include <math.h>
#include "vector2.h"

//播放音效
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
#define WIN_WIDTH 900
#define WIN_HEIGHT 600
#define ZOM_MAX 10
/*
1.创建新项目
2.导入素材
5.实现工具栏中的植物卡牌
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


int curX, curY;//当前选中的植物，在移动过程中的位置
int curPLANTS;//0:没有选中植物 1:选择了第一种植物\
//植物结构体
struct Plants
{
	int type;	//0:没有植物,1:表示第一种
	int frameIndex;//序列帧序号
	bool catched;//是否被僵尸捕获
	int deadTimer; //死亡计时器
	int timer;
	int x, y;//植物坐标
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
	bool eating;//正在吃植物
};
struct Zom zoms[10];
IMAGE imgZOM[22];
IMAGE imgZomDead[20];
IMAGE imgZomEat[21];
//定义子弹
struct bullet
{
	int x, y;
	int row;
	bool used;
	int speed;
	//是否发生爆炸
	bool blast;
	int frameIndex;//帧序号
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
	int frameIndex;//当前显示图片帧序号
	int destY;//飘落的目标位置的y坐标
	bool used;//是否在使用
	int timer;//定时器
	//偏移量
	float xoff;
	float yoff;

	float t;//贝塞尔曲线的时间点0..1
	vector2 p1, p2, p3, p4;
	vector2 pCur;//当前时刻阳光球的位置
	float speed;
	int status;
};

//后端开发，池化思想.....
struct sunshineBall balls[10];
IMAGE imgSunShineBall[29];
int SunShine;


//判断文件是否存在函数
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
	//游戏初始化,加载游戏背景图片
	//字符集改为多字节字符集
	loadimage(&imBg,"res/bg.jpg");
	loadimage(&imBar, "res/bar5.png");
	//开辟一块内存
	memset(imgPLANTS, 0, sizeof(imgPLANTS));
	memset(map, 0, sizeof(map));
	
	killCount = 0;
	zomCount = 0;
	gameStatus = GOING;


	//初始化植物卡牌
	char name[64];
	for (int i = 0; i < PLANTS_COUNTS; i++)
	{
		sprintf_s(name,sizeof(name),"res/Cards/card_%d.png",i+1);
		loadimage(&imgCards[i], name);

		for (int j = 0; j < 20; j++)
		{
		//初始化植物
		//这里要注意，不要复制上面的输出error否则会报错
		sprintf_s(name, sizeof(name),"res/zhiwu/%d/%d.png",i,j + 1);
		//先判断文件是否存在
		if (fileExist(name)) {
			//先分配内存，c++分配方式，C语言用malloc

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

	//创建游戏窗口
	curPLANTS = 0;
	SunShine = 50;
	//清除池中ball为0
	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunShineBall[i], name);
	}
	srand(time(NULL));

	initgraph(WIN_WIDTH, WIN_HEIGHT,1);
	//显示阳光值
	//输出文本
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	//微软雅黑
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;//抗锯齿
	settextstyle(&f);
	//设置背景颜色透明
	setbkmode(TRANSPARENT);
	//设置字体颜色
	setcolor(BLACK);

	//初始化僵尸数据
	memset(zoms, 0, sizeof(zoms));
	for (int i = 0; i < 22; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZOM[i], name);
	}

	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));
	//初始化豌豆子弹的帧图片数组
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
//渲染僵尸
void drawZom() {
	int zomCount = sizeof(zoms) / sizeof(zoms[0]);
	for (int i = 0; i < zomCount; i++)
	{	
		if (zoms[i].used)
		{
			IMAGE* img = NULL;
			//判断僵尸状态执行渲染动画
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
//绘制阳光
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

	//拖动过程中的植物
	if (curPLANTS > 0) {
		IMAGE* img = imgPLANTS[curPLANTS - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);
	}
}

void drawBullets() {
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int i = 0;
	for (i = 0; i < bulletMax; i++) {
		//首先判断是否使用
		if (bullets[i].used)
		{	//如果状态是blast，渲染爆炸
			if (bullets[i].blast)
			{
				IMAGE* img = &imgBallBlast[bullets[i].frameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
				FlushBatchDraw();
			}
			else//否则渲染射击
			{
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}
		}
	}
}
//渲染图片
void updateWindow() {
	//双缓冲
	BeginBatchDraw();//开始缓冲
	putimage(0, 0, &imBg);
	putimagePNG(250, 0, &imBar);
	drawCards();
	//种植植物
	drawPlants();
	//渲染阳光
	drawSunShines();
	drawZom();
	//drawBullet();
	drawBullets();
	EndBatchDraw();
}

//获取阳光
void collectSunShine(ExMessage* msg) {
	int sunshinecount = sizeof(balls) / sizeof(balls[0]);
	//获取阳光的坐标
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
				//播放音乐函数
				//mciSendString("play res/sunshine.mp3",0,0,0);
				PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);
				//设置阳光球的偏移量
				//float destY = 0;
				//float destX = 262;
				//float angle = atan((y - destY) / (x - destX));
				//balls[i].xoff = 4 * cos(angle);
				//balls[i].yoff = 4 * sin(angle);
				//绘制贝塞曲线
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
	//判断鼠标状态函数
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) {
		//当左键单击的时候,鼠标的范围
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
			//获得渲染位置
			curX = msg.x;
			curY = msg.y;
			//确定渲染哪一种植物

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
					//计算植物位置
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

		//从阳光池中取一个可以使用的
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
		//贝塞曲线方法创建阳光
		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(260 + rand() % 900 - 260, 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		int off = 2;
		float distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.0 / (distance / off);

	}
		//向日葵生成阳光
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
//创建僵尸
void createZom() {
	if (zomCount >= ZOM_MAX)
	{
		return;
	}
	//生成频率
	static int zomFre = 200;
	static int count = 0;

	count++;
	if (count > zomFre)
	{
		count = 0;
		zomFre = 300 + rand() % 200 ;

		int i;
		int zomMax = sizeof(zoms) / sizeof(zoms[0]);
		//注意这里for不可以括住下面的if
		for (i = 0; i < zomMax && zoms[i].used; i++);
			if (i < zomMax)
			{
				memset(&zoms[i], 0, sizeof(zoms[i]));
				zoms[i].used = true;
				zoms[i].x = WIN_WIDTH;
				zoms[i].row = rand() % 3;
				zoms[i].y = 172 + (1 + zoms[i].row) * 100;
				zoms[i].speed = 1;
				//僵尸碰撞检测
				zoms[i].blood = 100;
				zoms[i].dead = false;
				zomCount++;
			}
	}
	
}

void updateZom() {
	//获得僵尸数量
	int zomMax = sizeof(zoms) / sizeof(zoms[0]);
	static int count = 0;
	count++;
	if (count > 4)
	{
		count = 0;
		//更新位置
		for (int i = 0; i < zomMax; i++)
		{
			if (zoms[i].used) {

				zoms[i].x -= zoms[i].speed;
				if (zoms[i].x < 160)
				{
					//printf("game over \n");
					//MessageBox(NULL, "over", "over", 0);//待优化
					//exit(0);//游戏结束优化
					gameStatus = FAIL;
				}
			}
		}
	}
	
	//让僵尸动起来
	static int count2 = 0;
	count2++;
	if (count2 > 4)
	{
		count2 = 0;

		for (int i = 0; i < zomMax; i++)
		{	//僵尸死亡播放动画
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
				//僵尸吃植物播放
				else if (zoms[i].eating)
				{
					zoms[i].frameIndex = (zoms[i].frameIndex + 1) % 21;
				}//僵尸行走播放
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
	//判断这一排有无僵尸
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
					//循环生成子弹射击
					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);
					if (k < bulletMax)
					{
						bullets[k].used = true;
						//注意这里是i，这里如果用k就会射歪来
						bullets[k].row = i;
						bullets[k].speed = 6 ;
						bullets[k].blast = false;
						bullets[k].frameIndex = 0;
						//int x = 256 + j * 81;
						//int y = 170 + i * 95;
						int PlantsX = 256 + j * 81;
						int PlantsY = 170 + i * 95+14;
						//子弹偏移量
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
			//待实现子弹的碰撞检测
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
//实现子弹碰撞检测

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
	checkBullet2Zom();//子弹对僵尸碰撞检测
	checkZom2Plants();//僵尸对植物的碰撞检测
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

//循环渲染植物，让植物动起来
void updateGame() {
	updatePlants();
	createSunShine();
	//更新阳光状态
	updateSunShine();
	//创建僵尸
	createZom();
	//更新僵尸状态
	updateZom();
	
	//发射子弹
	shoot();
	updateBullet();
	collisionCheck();//碰撞检查
}
//制作开始菜单
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
		//获取延时时间函数
		//从用户点击鼠标开始计时
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

