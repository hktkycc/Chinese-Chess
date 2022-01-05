#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include<stdlib.h>
#include <time.h> 
#include<vector>
#include<map>
#include<string>
#include<graphics.h>
#include<conio.h>
#include<windows.h>
#include<fstream>
#pragma comment(lib,"ws2_32.lib")
class UI;
class Chess;
class ChessBoard;
class ChessSocketClient;
class ChessSocketServr;
using namespace std;

#define BUTTONTEXT RGB(30,30,30)
#define OVERBUTTON RGB(60,60,60)
#define BUTTONCOLOR RGB(200,130,100)
#define BGColor RGB(220,170,50)
#define Height 500
#define Width 452
#define Margin 30
#define GZ 49
#define ChessSize 20

bool isTurn = 1;
fstream Tlog;
int mode = 0;
void DrawFisrt();

struct InMessage
{
	InMessage(int x, int y, int i, int j)
	{
		this->x = x;
		this->y = y;
		this->i = i;
		this->j = j;
	}
	int x;
	int y;
	int i;
	int j;
};

class ChessSocketClient
{
public:
	friend UI;
	ChessSocketClient(string s)
	{
		log.open("SockLogClient.txt", ios::out | ios::trunc);
		Initial();
		Connect(s);
		char buf;
		recv(hSock, &buf, sizeof(char), 0);
		if (buf == true)
			isTurn = 0;
		log << "是否先手：" << isTurn << endl;
	}
	~ChessSocketClient()
	{
		log.close();
	}
	SOCKET hSock;

private:
	ofstream log;
	WORD w_req = MAKEWORD(2, 2);//版本号
	//加载套接字
	WSADATA wsadata;
	SOCKADDR_IN serverAddr;
	void Initial()
	{//初始化套接字库
		if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
		{
			log << "WSAStartup error" << endl;
		}
		hSock = socket(PF_INET, SOCK_STREAM, 0);
		if (hSock == INVALID_SOCKET)
		{
			log << "Socket Startup error" << endl;
		}
	}
	bool Connect(string s)
	{
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = inet_addr((const char*)&s);
		serverAddr.sin_port = htons(3456);
		if (connect(hSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			log << "connect error" << endl;
			return false;
		}
		else
			return true;
	}
};

class ChessSocketServer
{
public:
	friend UI;
	ChessSocketServer()
	{
		log.open("SockLogServer.txt", ios::out | ios::trunc);
		Initial();
		Listen();
		Connect();
		isTurn = rand() % 2;
		//isTurn = 1;
		send(hClient, (const char*)&isTurn, sizeof(char), 0);
		log << "是否先手：" << isTurn << endl;
	}
	~ChessSocketServer()
	{
		log.close();
	}
	SOCKET hServer;
	SOCKET hClient;

private:
	ofstream log;
	WORD w_req = MAKEWORD(2, 2);//版本号
	WSADATA wsadata;
	SOCKADDR_IN serverAddr;
	SOCKADDR_IN clientAddr;
	int szClntAddr;
	void Initial()
	{//初始化套接字库
		if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
		{
			log << "WSAStartup error" << endl;
		}
		hServer = socket(PF_INET, SOCK_STREAM, 0);
		if (hServer == INVALID_SOCKET)
		{
			log << "Socket Startup error" << endl;
		}
	}
	void Listen()
	{
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serverAddr.sin_port = htons(3456);
		if (bind(hServer, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			log << "Bind error" << endl;
		}
		if (listen(hServer, 5) == SOCKET_ERROR)
		{
			log << "listen error" << endl;
		}
	}
	void Connect()
	{
		szClntAddr = sizeof(clientAddr);
		hClient = accept(hServer, (SOCKADDR*)&clientAddr, &szClntAddr);
		if (hClient == INVALID_SOCKET)
		{
			log << "accept error" << endl;
		}
	}
};

class Chess//棋子类
{
public:
	Chess(int type, int posX, int posY)//构造函数
	{
		this->type = type;
		this->posX = posX;
		this->posY = posY;
	}

	bool operator==(const Chess& obj)
	{
		if (this->type == obj.type && this->posX == obj.posX && this->posY == obj.posY)
			return true;
	}

	int type;//棋子类型
	int posX, posY;//棋子位置
};

class ChessBoard//棋盘类
{
public:
	friend UI;
	friend ChessSocketClient;
	friend ChessSocketServer;
	ChessBoard()
	{
		height = 10;
		width = 9;
		Board.assign(height, vector<int>(width, 0));
		OnlineIsRed = isTurn;
		BoardInit();
		ChessInit();
		BoardUpdate();
	}
	void BoardUpdate()//将红黑两个列表更新入Board
	{
		Board.assign(height, vector<int>(width, 0));					//这里先对两个列表置零
		for (auto i : RedList)
			Board[i.posX][i.posY] = i.type;
		for (auto i : BlackList)
			Board[i.posX][i.posY] = i.type;
	}

	void initGraph()
	{
		initgraph(Width, Height);//初始化一个大小为Height*Width的界面
		drawMap();
		drawPiece();
	}

	bool game(ExMessage& operation)//游戏运行函数
	{
		if (operation.message == WM_LBUTTONDOWN)//如果左键按下，进行的处理程序
		{
			if (!isSelected)
				MessageProcessFirst(operation);//如果没有棋子被选择，变成选择棋子模式
			else
				MessageProcessSecond(operation);//若已有棋子被选择，进入移动模式
		}
		if (WinCheck())
		{
			return true;
		}
		return false;
	}

private:
	int height, width;//棋盘大小，固定为8*9
	vector<vector<int>> Board;//棋盘，存储棋盘中每个点的状态
	map<int, string> reflex;//棋子的类型与汉字名称对应表
	vector<Chess> RedList, BlackList;//保存红方和黑方的剩余棋子
	int SelectX = -1, SelectY = -1;//选择到的棋子的坐标
	int SocketX = -1, SocketY = -1;//移动前的坐标点，供socket使用
	bool isSelected;//是否有棋子被选择
	bool isRed = true;
	bool OnlineIsRed;
	bool WasRule = false;

	void BoardInit()//建立棋子类型名称对应表
	{
		//reflex[0] = "   ";
		reflex[1] = "帅";
		reflex[2] = "仕";
		reflex[3] = "相";
		reflex[4] = "马";
		reflex[5] = "车";
		reflex[6] = "仕";
		reflex[7] = "相";
		reflex[8] = "马";
		reflex[9] = "车";
		reflex[10] = "炮";
		reflex[11] = "炮";
		reflex[12] = "卒";
		reflex[13] = "卒";
		reflex[14] = "卒";
		reflex[15] = "卒";
		reflex[16] = "卒";

		reflex[17] = "将";
		reflex[18] = "士";
		reflex[19] = "象";
		reflex[20] = "馬";
		reflex[21] = "車";
		reflex[22] = "士";
		reflex[23] = "象";
		reflex[24] = "馬";
		reflex[25] = "車";
		reflex[26] = "砲";
		reflex[27] = "砲";
		reflex[28] = "兵";
		reflex[29] = "兵";
		reflex[30] = "兵";
		reflex[31] = "兵";
		reflex[32] = "兵";
	}
	void ChessInit()//棋子初始化
	{
		RedList.emplace_back(1, 0, 4);
		RedList.emplace_back(2, 0, 3);
		RedList.emplace_back(6, 0, 5);
		RedList.emplace_back(3, 0, 2);
		RedList.emplace_back(7, 0, 6);
		RedList.emplace_back(4, 0, 1);
		RedList.emplace_back(8, 0, 7);
		RedList.emplace_back(5, 0, 0);
		RedList.emplace_back(9, 0, 8);
		RedList.emplace_back(10, 2, 1);
		RedList.emplace_back(11, 2, 7);
		RedList.emplace_back(12, 3, 0);
		RedList.emplace_back(13, 3, 2);
		RedList.emplace_back(14, 3, 4);
		RedList.emplace_back(15, 3, 6);
		RedList.emplace_back(16, 3, 8);

		BlackList.emplace_back(17, 9, 4);
		BlackList.emplace_back(18, 9, 3);
		BlackList.emplace_back(22, 9, 5);
		BlackList.emplace_back(19, 9, 2);
		BlackList.emplace_back(23, 9, 6);
		BlackList.emplace_back(20, 9, 1);
		BlackList.emplace_back(24, 9, 7);
		BlackList.emplace_back(21, 9, 0);
		BlackList.emplace_back(25, 9, 8);
		BlackList.emplace_back(26, 7, 1);
		BlackList.emplace_back(27, 7, 7);
		BlackList.emplace_back(28, 6, 0);
		BlackList.emplace_back(29, 6, 2);
		BlackList.emplace_back(30, 6, 4);
		BlackList.emplace_back(31, 6, 6);
		BlackList.emplace_back(32, 6, 8);
	}

	bool WinCheck()
	{
		int ret = 0;
		if (RedList[0].type != 1 || RedList.size() == 0)
		{
			ret = 1;
		}
		else if (BlackList[0].type != 17 || BlackList.size() == 0)
		{
			ret = 2;
		}
		if (ret != 0)
		{
			int checkHeight = 300;
			int checkWidth = 400;
			initgraph(checkWidth, checkHeight);
			IMAGE background;//定义一个图片名.
			loadimage(&background, "checkmate.png", checkWidth, checkHeight, 1);//从图片文件获取图像
			putimage(0, 0, &background);

			settextcolor(RGB(240, 230, 30));//红底金色的结算界面
			LOGFONT f;
			gettextstyle(&f);						// 获取当前字体设置
			f.lfHeight = 40;						// 设置字体高度为 48
			_tcscpy(f.lfFaceName, _T("黑体"));		// 设置字体为“黑体”(高版本 VC 推荐使用 _tcscpy_s 函数)
			f.lfQuality = ANTIALIASED_QUALITY;		// 设置输出效果为抗锯齿
			setbkmode(TRANSPARENT);
			settextstyle(&f);

			if (ret == 1)
				outtextxy((checkWidth) / 2 - 80, checkHeight / 2 - 40, _T("黑方胜利"));
			else if (ret == 2)
				outtextxy((checkWidth) / 2 - 80, checkHeight / 2 - 40, _T("红方胜利"));

			f.lfHeight = 30;						// 设置字体高度为 48
			settextstyle(&f);
			settextcolor(WHITE);
			outtextxy(checkWidth / 2 - 135, checkHeight / 2 + 60, _T("按下回车键重新开始"));
			outtextxy(checkWidth / 2 - 135, checkHeight / 2 + 100, _T("按下ESC键退出"));
			ExMessage isDown;
			while (1)
			{
				peekmessage(&isDown, -1, 1);
				if (isDown.vkcode == VK_RETURN)
				{
					restart();
					break;
				}
				else if (isDown.vkcode == VK_ESCAPE)
				{
					return true;
				}
			}
		}
		return false;
	}
	bool OnlineWinCheck()
	{
		int ret = 0;
		if (RedList[0].type != 1 || RedList.size() == 0)
		{
			ret = 1;
		}
		else if (BlackList[0].type != 17 || BlackList.size() == 0)
		{
			ret = 2;
		}
		if (ret != 0)
		{
			int checkHeight = 300;
			int checkWidth = 400;
			initgraph(checkWidth, checkHeight);
			IMAGE background;//定义一个图片名.
			loadimage(&background, "checkmate.png", checkWidth, checkHeight, 1);//从图片文件获取图像
			putimage(0, 0, &background);

			settextcolor(RGB(240, 230, 30));//红底金色的结算界面
			LOGFONT f;
			gettextstyle(&f);						// 获取当前字体设置
			f.lfHeight = 40;						// 设置字体高度为 48
			_tcscpy(f.lfFaceName, _T("黑体"));		// 设置字体为“黑体”(高版本 VC 推荐使用 _tcscpy_s 函数)
			f.lfQuality = ANTIALIASED_QUALITY;		// 设置输出效果为抗锯齿
			setbkmode(TRANSPARENT);
			settextstyle(&f);

			if (ret == 1)
				outtextxy((checkWidth) / 2 - 80, checkHeight / 2 - 40, _T("黑方胜利"));
			else if (ret == 2)
				outtextxy((checkWidth) / 2 - 80, checkHeight / 2 - 40, _T("红方胜利"));

			f.lfHeight = 30;						// 设置字体高度为 48
			settextstyle(&f);
			settextcolor(WHITE);
			outtextxy(checkWidth / 2 - 135, checkHeight / 2 + 60, _T("按下回车键重新开始"));
			outtextxy(checkWidth / 2 - 135, checkHeight / 2 + 100, _T("按下ESC键退出"));
			ExMessage isDown;
			while (1)
			{
				peekmessage(&isDown, -1, 1);
				if (isDown.vkcode == VK_RETURN)
				{
					void DrawFisrt();
					if (OnlineRestart())
					{
						break;
					}
				}
				else if (isDown.vkcode == VK_ESCAPE)
				{
					return true;
				}
			}
		}
		return false;
	}

	void drawMap()//绘制背景棋盘
	{
		IMAGE background;//定义一个图片名.
		loadimage(&background, "images.png", 452, 500, 1);//从图片文件获取图像
		putimage(0, 0, &background);
	}
	void drawPiece()//绘制棋子
	{
		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
				if (Board[i][j] != 0)
				{
					if (Board[i][j] > 16)
					{
						setlinecolor(BLACK);
						settextcolor(BLACK);
					}
					else
					{
						setlinecolor(RED);
						settextcolor(RED);
					}
					TCHAR s[8];
					_stprintf(s, _T("%s"), reflex[Board[i][j]]);
					setfillcolor(BGColor);
					fillcircle(Margin + GZ * j, Margin + GZ * i, ChessSize);
					LOGFONT f;
					gettextstyle(&f);						// 获取当前字体设置
					f.lfHeight = 20;						// 设置字体高度为 48
					_tcscpy(f.lfFaceName, _T("黑体"));		// 设置字体为“黑体”(高版本 VC 推荐使用 _tcscpy_s 函数)
					f.lfQuality = ANTIALIASED_QUALITY;		// 设置输出效果为抗锯齿
					setbkmode(TRANSPARENT);
					settextstyle(&f);
					outtextxy(Margin + GZ * j - 10, Margin + GZ * i - 10, s);
				}
	}
	void MessageProcessFirst(ExMessage operation)//选择模式处理函数
	{
		uint32_t posx, posy;
		int mx, my;
		mx = (operation.x - Margin - GZ / 2);
		my = (operation.y - Margin - GZ / 2);
		if (mx > 0)
			posx = Margin + GZ * ((mx / GZ) + 1);
		else
		{
			posx = Margin;
			mx = -GZ;
		}
		if (my > 0)
			posy = Margin + GZ * ((my / GZ) + 1);
		else
		{
			posy = Margin;
			my = -GZ;
		}
		if (SelectX == (my / GZ + 1) && SelectY == (mx / GZ + 1))
		{
			isSelected = 0;
			return;
		}
		if (Board[my / GZ + 1][mx / GZ + 1] != 0 && Board[my / GZ + 1][mx / GZ + 1] <= 16 && isRed)
		{
			SelectPiece(posx, posy);
			isSelected = 1;
			SelectX = my / GZ + 1;
			SelectY = mx / GZ + 1;
		}
		else if (Board[my / GZ + 1][mx / GZ + 1] > 16 && !isRed)
		{
			SelectPiece(posx, posy);
			isSelected = 1;
			SelectX = my / GZ + 1;
			SelectY = mx / GZ + 1;
		}
		SocketX = my / GZ + 1;
		SocketY = mx / GZ + 1;
	}
	void MessageProcessSecond(ExMessage operation)//移动模式处理函数
	{
		uint32_t posx, posy;
		int mx, my;
		mx = (operation.x - Margin - GZ / 2);
		my = (operation.y - Margin - GZ / 2);
		if (mx > 0)
			posx = Margin + GZ * ((mx / GZ) + 1);
		else
		{
			posx = Margin;
			mx = -GZ;
		}
		if (my > 0)
			posy = Margin + GZ * ((my / GZ) + 1);
		else
		{
			posy = Margin;
			my = -GZ;
		}
		int i = my / GZ + 1;
		int j = mx / GZ + 1;
		if (SelectX == i && SelectY == j)
		{
			isSelected = 1;
			return;
		}
		movePiece(SelectX, SelectY, i, j, operation);
		SelectX = i;
		SelectY = j;
	}
	void OnlineMessageProcessFirst(ExMessage operation)//选择模式处理函数
	{
		uint32_t posx, posy;
		int mx, my;
		mx = (operation.x - Margin - GZ / 2);
		my = (operation.y - Margin - GZ / 2);
		if (mx > 0)
			posx = Margin + GZ * ((mx / GZ) + 1);
		else
		{
			posx = Margin;
			mx = -GZ;
		}
		if (my > 0)
			posy = Margin + GZ * ((my / GZ) + 1);
		else
		{
			posy = Margin;
			my = -GZ;
		}
		if (SelectX == (my / GZ + 1) && SelectY == (mx / GZ + 1))
		{
			isSelected = 0;
			return;
		}
		if (Board[my / GZ + 1][mx / GZ + 1] != 0 && Board[my / GZ + 1][mx / GZ + 1] <= 16 && OnlineIsRed)
		{
			SelectPiece(posx, posy);
			isSelected = 1;
			SelectX = my / GZ + 1;
			SelectY = mx / GZ + 1;
		}
		else if (Board[my / GZ + 1][mx / GZ + 1] > 16 && !OnlineIsRed)
		{
			SelectPiece(posx, posy);
			isSelected = 1;
			SelectX = my / GZ + 1;
			SelectY = mx / GZ + 1;
		}
		SocketX = my / GZ + 1;
		SocketY = mx / GZ + 1;
	}
	void OnlineMessageProcessSecond(ExMessage operation)//移动模式处理函数
	{
		uint32_t posx, posy;
		int mx, my;
		mx = (operation.x - Margin - GZ / 2);
		my = (operation.y - Margin - GZ / 2);
		if (mx > 0)
			posx = Margin + GZ * ((mx / GZ) + 1);
		else
		{
			posx = Margin;
			mx = -GZ;
		}
		if (my > 0)
			posy = Margin + GZ * ((my / GZ) + 1);
		else
		{
			posy = Margin;
			my = -GZ;
		}
		int i = my / GZ + 1;
		int j = mx / GZ + 1;
		if (SelectX == i && SelectY == j)
		{
			isSelected = 1;
			WasRule = false;
			return;
		}
		movePiece(SelectX, SelectY, i, j, operation);
		SelectX = i;
		SelectY = j;
	}
	void SelectPiece(int posx, int posy)//对选择到的棋子绘制蓝框表示
	{
		BeginBatchDraw();
		FlushBatchDraw();

		drawMap();
		drawPiece();
		setlinecolor(BLUE);
		rectangle(posx - 20, posy - 20, posx + 20, posy + 20);
		EndBatchDraw();
	}
	void movePiece(int x, int y, int i, int j, ExMessage& operation)//移动棋子函数
	{
		BeginBatchDraw();
		FlushBatchDraw();
		if (Board[x][y] <= 16 && Board[x][y] != 0)
		{
			if (Board[i][j] == 0)
			{
				for (int m = 0; m < RedList.size(); m++)
				{
					if (RedList[m].type == Board[x][y])
					{
						if (Rule(x, y, i, j))
						{
							WasRule = 1;
							RedList[m].posX = i;
							RedList[m].posY = j;
							isRed = !isRed;
							break;
						}
						else
							WasRule = 0;
					}
				}
			}
			else if (Board[i][j] > 16)
			{
				for (int m = 0; m < RedList.size(); m++)
				{
					if (RedList[m].type == Board[x][y])
					{
						if (Rule(x, y, i, j))
						{
							WasRule = 1;
							RedList[m].posX = i;
							RedList[m].posY = j;
							for (int n = 0; n < BlackList.size(); n++)
							{
								if (BlackList[n].type == Board[i][j])
								{
									BlackList.erase(BlackList.begin() + n);
									isRed = !isRed;
									break;
								}
							}
							break;
						}
						else
							WasRule = 0;
					}
				}
			}
		}
		else if (Board[x][y] > 16 && Board[x][y] != 0)
		{
			if (Board[i][j] == 0)
			{
				for (int m = 0; m < BlackList.size(); m++)
				{
					if (BlackList[m].type == Board[x][y])
					{
						if (Rule(x, y, i, j))
						{
							WasRule = 1;
							BlackList[m].posX = i;
							BlackList[m].posY = j;
							isRed = !isRed;
							break;
						}
						else
							WasRule = 0;
					}
				}
			}
			else if (Board[i][j] <= 16)
			{
				for (int m = 0; m < BlackList.size(); m++)
				{
					if (BlackList[m].type == Board[x][y])
					{
						if (Rule(x, y, i, j))
						{
							WasRule = 1;
							BlackList[m].posX = i;
							BlackList[m].posY = j;
							for (int n = 0; n < RedList.size(); n++)
							{
								if (RedList[n].type == Board[i][j])
								{
									RedList.erase(RedList.begin() + n);
									isRed = !isRed;
									break;
								}
							}
							break;
						}
						else
							WasRule = 0;
					}
				}
			}
		}
		isSelected = 0;
		BoardUpdate();
		cleardevice();
		drawMap();
		drawPiece();
		EndBatchDraw();
		if (mode == 1)
			Tlog << SocketX << " " << SocketY << " " << SelectX << " " << SelectY << " " << WasRule << endl;
	}
	bool Rule(int x, int y, int i, int j)
	{
		bool ret = false;
		switch (Board[x][y])
		{
		case 1://帅的走棋规则
			if (i <= 2)
				if (j >= 3 && j <= 5)//限定田字格
					if ((abs(j - y) == 1 && x == i) || (abs(i - x) == 1 && j == y))//限定一次一步且横平竖直
						ret = true;
			if (BlackList[0].posY == y)//判定双将见面，可以吃掉对方的将
			{
				bool isBlock = false;
				for (int m = x + 1; m < BlackList[0].posX; m++)
					if (Board[m][y] != 0)
						isBlock = true;
				if (!isBlock)
					if (i == BlackList[0].posX && j == BlackList[0].posY)
						ret = true;
			}
			break;
		case 17://将的规则，同上
			if (i >= 7)
				if (j >= 3 && j <= 5)
					if ((abs(j - y) == 1 && x == i) || (abs(i - x) == 1 && j == y))
						ret = true;
			if (RedList[0].posY == y)
			{
				bool isBlock = false;
				for (int m = x + 1; m < RedList[0].posX; m++)
					if (Board[m][y] != 0)
						isBlock = true;
				if (!isBlock)
					if (i == RedList[0].posX && j == RedList[0].posY)
						ret = true;
			}
			break;
		case 2://仕的规则
			if (i <= 2)
			{
				if (j >= 3 && j <= 5)//限定田字格
				{
					if (abs(j - y) == 1 && abs(i - x) == 1)//限定斜线
						ret = true;
				}
			}
			break;
		case 6://仕的规则2，同上
			if (i <= 2)
			{
				if (j >= 3 && j <= 5)
				{
					if (abs(j - y) == 1 && abs(i - x) == 1)
						ret = true;
				}
			}
			break;
		case 3://相的规则
			if (i <= 4)
			{
				if (abs(j - y) == 2 && abs(i - x) == 2)//保证走田字
				{//保证不被别相眼
					if (j > y && i > x && Board[i - 1][j - 1] == 0)
						ret = true;
					if (j < y && i > x && Board[i - 1][j + 1] == 0)
						ret = true;
					if (j > y && i < x && Board[i + 1][j - 1] == 0)
						ret = true;
					if (j < y && i < x && Board[i + 1][j + 1] == 0)
						ret = true;
				}
			}
			break;
		case 7://相的规则2，同上
			if (i <= 4)
			{
				if (abs(j - y) == 2 && abs(i - x) == 2)
				{
					if (j > y && i > x && Board[i - 1][j - 1] == 0)
						ret = true;
					if (j < y && i > x && Board[i - 1][j + 1] == 0)
						ret = true;
					if (j > y && i < x && Board[i + 1][j - 1] == 0)
						ret = true;
					if (j < y && i < x && Board[i + 1][j + 1] == 0)
						ret = true;
				}
			}
			break;
		case 4://马的规则
			if (abs(j - y) == 2 && abs(i - x) == 1)//水平日字
			{
				if (j > y && Board[x][j - 1] == 0)//保证不别马腿
					ret = true;
				if (j < y && Board[x][j + 1] == 0)
					ret = true;
			}
			else if (abs(j - y) == 1 && abs(i - x) == 2)//竖直日字
			{
				if (i > x && Board[i - 1][y] == 0)
					ret = true;
				if (i < x && Board[i + 1][y] == 0)
					ret = true;
			}
			break;
		case 8://马的规则2，同上
			if (abs(j - y) == 2 && abs(i - x) == 1)
			{
				if (j > y && Board[x][j - 1] == 0)
					ret = true;
				if (j < y && Board[x][j + 1] == 0)
					ret = true;
			}
			else if (abs(j - y) == 1 && abs(i - x) == 2)
			{
				if (i > x && Board[i - 1][y] == 0)
					ret = true;
				if (i < x && Board[i + 1][y] == 0)
					ret = true;
			}
			break;
		case 5://车的规则
			if (j == y)//竖直方向走
			{
				bool isBlock = false;
				if (i > x)//前进
				{
					for (int m = x + 1; m < i; m++)
						if (Board[m][y] != 0)
							isBlock = true;
				}
				else if (i < x)//后退
				{
					for (int m = x - 1; m > i; m--)
						if (Board[m][y] != 0)
							isBlock = true;
				}
				if (!isBlock)
					ret = true;
			}
			else if (i == x)//水平方向走
			{
				bool isBlock = false;
				if (j > y)//向右
				{
					for (int m = y + 1; m < j; m++)
						if (Board[x][m] != 0)
							isBlock = true;
				}
				else if (j < y)//向左
				{
					for (int m = y - 1; m > j; m--)
						if (Board[x][m] != 0)
							isBlock = true;
				}
				if (!isBlock)
					ret = true;
			}
			break;
		case 9://车的规则2，同上
			if (j == y)
			{
				bool isBlock = false;
				if (i > x)
				{
					for (int m = x + 1; m < i; m++)
						if (Board[m][y] != 0)
							isBlock = true;
				}
				else if (i < x)
				{
					for (int m = x - 1; m > i; m--)
						if (Board[m][y] != 0)
							isBlock = true;
				}
				if (!isBlock)
					ret = true;
			}
			else if (i == x)
			{
				bool isBlock = false;
				if (j > y)
				{
					for (int m = y + 1; m < j; m++)
						if (Board[x][m] != 0)
							isBlock = true;
				}
				else if (j < y)
				{
					for (int m = y - 1; m > j; m--)
						if (Board[x][m] != 0)
							isBlock = true;
				}
				if (!isBlock)
					ret = true;
			}
			break;
		case 10://炮的规则
			if (j == y)//基本与车的规则相同，在吃子还是移动时判别
			{
				int cnt = 0;
				if (i > x)
				{
					for (int m = x + 1; m < i; m++)
						if (Board[m][y] != 0)
							cnt++;
				}
				else if (i < x)
				{
					for (int m = x - 1; m > i; m--)
						if (Board[m][y] != 0)
							cnt++;
				}
				if (Board[i][j] == 0 && cnt == 0)//仅移动时，中间不可以有子
					ret = true;
				else if (Board[i][j] != 0 && cnt == 1)//吃子时，中间必须有一个子
					ret = true;
			}
			else if (i == x)//水平移动同上
			{
				int cnt = 0;
				if (j > y)
				{
					for (int m = y + 1; m < j; m++)
						if (Board[x][m] != 0)
							cnt++;
				}
				else if (j < y)
				{
					for (int m = y - 1; m > j; m--)
						if (Board[x][m] != 0)
							cnt++;
				}
				if (Board[i][j] == 0 && cnt == 0)
					ret = true;
				else if (Board[i][j] != 0 && cnt == 1)
					ret = true;
			}
			break;
		case 11://炮的规则2，同上
			if (j == y)
			{
				int cnt = 0;
				if (i > x)
				{
					for (int m = x + 1; m < i; m++)
						if (Board[m][y] != 0)
							cnt++;
				}
				else if (i < x)
				{
					for (int m = x - 1; m > i; m--)
						if (Board[m][y] != 0)
							cnt++;
				}
				if (Board[i][j] == 0 && cnt == 0)
					ret = true;
				else if (Board[i][j] != 0 && cnt == 1)
					ret = true;
			}
			else if (i == x)
			{
				int cnt = 0;
				if (j > y)
				{
					for (int m = y + 1; m < j; m++)
						if (Board[x][m] != 0)
							cnt++;
				}
				else if (j < y)
				{
					for (int m = y - 1; m > j; m--)
						if (Board[x][m] != 0)
							cnt++;
				}
				if (Board[i][j] == 0 && cnt == 0)
					ret = true;
				else if (Board[i][j] != 0 && cnt == 1)
					ret = true;
			}
			break;
		case 12://兵的规则
			if (x <= 4)
			{
				if (j == y && i - x == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (i - x == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		case 13://兵的规则2，同上
			if (x <= 4)
			{
				if (j == y && i - x == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (i - x == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		case 14://兵的规则3，同上
			if (x <= 4)
			{
				if (j == y && i - x == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (i - x == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		case 15://兵的规则4，同上
			if (x <= 4)
			{
				if (j == y && i - x == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (i - x == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		case 16://兵的规则5，同上
			if (x <= 4)
			{
				if (j == y && i - x == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (i - x == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		case 18://士的规则
			if (i >= 7)
			{
				if (j >= 3 && j <= 5)//限定田字格
				{
					if (abs(j - y) == 1 && abs(i - x) == 1)//限定斜线
						ret = true;
				}
			}
			break;
		case 22://士的规则2，同上
			if (i >= 7)
			{
				if (j >= 3 && j <= 5)
				{
					if (abs(j - y) == 1 && abs(i - x) == 1)
						ret = true;
				}
			}
			break;
		case 19://象的规则
			if (i >= 5)
			{
				if (abs(j - y) == 2 && abs(i - x) == 2)//保证走田字
				{//保证不被别相眼
					if (j > y && i > x && Board[i - 1][j - 1] == 0)
						ret = true;
					if (j < y && i > x && Board[i - 1][j + 1] == 0)
						ret = true;
					if (j > y && i < x && Board[i + 1][j - 1] == 0)
						ret = true;
					if (j < y && i < x && Board[i + 1][j + 1] == 0)
						ret = true;
				}
			}
			break;
		case 23://象的规则2，同上
			if (i >= 5)
			{
				if (abs(j - y) == 2 && abs(i - x) == 2)
				{
					if (j > y && i > x && Board[i - 1][j - 1] == 0)
						ret = true;
					if (j < y && i > x && Board[i - 1][j + 1] == 0)
						ret = true;
					if (j > y && i < x && Board[i + 1][j - 1] == 0)
						ret = true;
					if (j < y && i < x && Board[i + 1][j + 1] == 0)
						ret = true;
				}
			}
			break;
		case 20://馬的规则
			if (abs(j - y) == 2 && abs(i - x) == 1)//水平日字
			{
				if (j > y && Board[x][j - 1] == 0)//保证不别马腿
					ret = true;
				if (j < y && Board[x][j + 1] == 0)
					ret = true;
			}
			else if (abs(j - y) == 1 && abs(i - x) == 2)//竖直日字
			{
				if (i > x && Board[i - 1][y] == 0)
					ret = true;
				if (i < x && Board[i + 1][y] == 0)
					ret = true;
			}
			break;
		case 24://馬的规则2，同上
			if (abs(j - y) == 2 && abs(i - x) == 1)
			{
				if (j > y && Board[x][j - 1] == 0)
					ret = true;
				if (j < y && Board[x][j + 1] == 0)
					ret = true;
			}
			else if (abs(j - y) == 1 && abs(i - x) == 2)
			{
				if (i > x && Board[i - 1][y] == 0)
					ret = true;
				if (i < x && Board[i + 1][y] == 0)
					ret = true;
			}
			break;
		case 21://車的规则
			if (j == y)//竖直方向走
			{
				bool isBlock = false;
				if (i > x)//前进
				{
					for (int m = x + 1; m < i; m++)
						if (Board[m][y] != 0)
							isBlock = true;
				}
				else if (i < x)//后退
				{
					for (int m = x - 1; m > i; m--)
						if (Board[m][y] != 0)
							isBlock = true;
				}
				if (!isBlock)
					ret = true;
			}
			else if (i == x)//水平方向走
			{
				bool isBlock = false;
				if (j > y)//向右
				{
					for (int m = y + 1; m < j; m++)
						if (Board[x][m] != 0)
							isBlock = true;
				}
				else if (j < y)//向左
				{
					for (int m = y - 1; m > j; m--)
						if (Board[x][m] != 0)
							isBlock = true;
				}
				if (!isBlock)
					ret = true;
			}
			break;
		case 25://車的规则2，同上
			if (j == y)
			{
				bool isBlock = false;
				if (i > x)
				{
					for (int m = x + 1; m < i; m++)
						if (Board[m][y] != 0)
							isBlock = true;
				}
				else if (i < x)
				{
					for (int m = x - 1; m > i; m--)
						if (Board[m][y] != 0)
							isBlock = true;
				}
				if (!isBlock)
					ret = true;
			}
			else if (i == x)
			{
				bool isBlock = false;
				if (j > y)
				{
					for (int m = y + 1; m < j; m++)
						if (Board[x][m] != 0)
							isBlock = true;
				}
				else if (j < y)
				{
					for (int m = y - 1; m > j; m--)
						if (Board[x][m] != 0)
							isBlock = true;
				}
				if (!isBlock)
					ret = true;
			}
			break;
		case 26://砲的规则
			if (j == y)//基本与车的规则相同，在吃子还是移动时判别
			{
				int cnt = 0;
				if (i > x)
				{
					for (int m = x + 1; m < i; m++)
						if (Board[m][y] != 0)
							cnt++;
				}
				else if (i < x)
				{
					for (int m = x - 1; m > i; m--)
						if (Board[m][y] != 0)
							cnt++;
				}
				if (Board[i][j] == 0 && cnt == 0)//仅移动时，中间不可以有子
					ret = true;
				else if (Board[i][j] != 0 && cnt == 1)//吃子时，中间必须有一个子
					ret = true;
			}
			else if (i == x)//水平移动同上
			{
				int cnt = 0;
				if (j > y)
				{
					for (int m = y + 1; m < j; m++)
						if (Board[x][m] != 0)
							cnt++;
				}
				else if (j < y)
				{
					for (int m = y - 1; m > j; m--)
						if (Board[x][m] != 0)
							cnt++;
				}
				if (Board[i][j] == 0 && cnt == 0)
					ret = true;
				else if (Board[i][j] != 0 && cnt == 1)
					ret = true;
			}
			break;
		case 27://砲的规则2，同上
			if (j == y)
			{
				int cnt = 0;
				if (i > x)
				{
					for (int m = x + 1; m < i; m++)
						if (Board[m][y] != 0)
							cnt++;
				}
				else if (i < x)
				{
					for (int m = x - 1; m > i; m--)
						if (Board[m][y] != 0)
							cnt++;
				}
				if (Board[i][j] == 0 && cnt == 0)
					ret = true;
				else if (Board[i][j] != 0 && cnt == 1)
					ret = true;
			}
			else if (i == x)
			{
				int cnt = 0;
				if (j > y)
				{
					for (int m = y + 1; m < j; m++)
						if (Board[x][m] != 0)
							cnt++;
				}
				else if (j < y)
				{
					for (int m = y - 1; m > j; m--)
						if (Board[x][m] != 0)
							cnt++;
				}
				if (Board[i][j] == 0 && cnt == 0)
					ret = true;
				else if (Board[i][j] != 0 && cnt == 1)
					ret = true;
			}
			break;
		case 28://卒的规则
			if (x >= 5)
			{
				if (j == y && x - i == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (x - i == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		case 29://卒的规则2，同上
			if (x >= 5)
			{
				if (j == y && x - i == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (x - i == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		case 30://卒的规则3，同上
			if (x >= 5)
			{
				if (j == y && x - i == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (x - i == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		case 31://卒的规则4，同上
			if (x >= 5)
			{
				if (j == y && x - i == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (x - i == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		case 32://卒的规则5，同上
			if (x >= 5)
			{
				if (j == y && x - i == 1)
					ret = true;
			}
			else
			{
				if ((abs(j - y) == 1 && x == i) || (x - i == 1 && j == y))//限定一次一步且横平竖直, 并且保证不能后退
					ret = true;
			}
			break;
		default:ret = false; break;
		}
		return ret;
	}

	void restart()
	{
		RedList.clear();
		BlackList.clear();
		ChessInit();
		BoardUpdate();
		initGraph();
		isRed = true;
	}

	bool OnlineRestart()
	{
		RedList.clear();
		BlackList.clear();
		ChessInit();
		BoardUpdate();
		initGraph();
		OnlineIsRed = isTurn;
		return true;
	}
};

class UI
{
public:
	UI()
	{
		initgraph(400, 400);
		loadimage(&background, "background.png", 400, 400, 1);//从图片文件获取图像
		putimage(0, 0, &background);
		drawButton(100, 200, 300, 150);
		drawButton(100, 230, 300, 280);
		drawButton(100, 310, 300, 360);
		LOGFONT f;
		gettextstyle(&f);						// 获取当前字体设置
		f.lfHeight = 20;						// 设置字体高度为 20
		_tcscpy(f.lfFaceName, _T("黑体"));		// 设置字体为“黑体”(高版本 VC 推荐使用 _tcscpy_s 函数)
		f.lfQuality = ANTIALIASED_QUALITY;		// 设置输出效果为抗锯齿
		setbkmode(TRANSPARENT);
		settextstyle(&f);
		settextcolor(BUTTONTEXT);
		ZhuJi();
		KeHu();
		ReZuo();
	}
	bool Process(ExMessage& operation)//鼠标消息处理
	{
		if (operation.message == WM_LBUTTONDOWN)
		{
			if (operation.x >= 100 && operation.x <= 300 && operation.y <= 200 && operation.y >= 150)
			{
				//这里添加启动服务器程序代码
				mode = 1;
				initgraph(200, 100);
				LOGFONT f;
				gettextstyle(&f);						// 获取当前字体设置
				f.lfHeight = 30;						// 设置字体高度为 20
				_tcscpy(f.lfFaceName, _T("黑体"));		// 设置字体为“黑体”(高版本 VC 推荐使用 _tcscpy_s 函数)
				f.lfQuality = ANTIALIASED_QUALITY;		// 设置输出效果为抗锯齿
				setbkmode(TRANSPARENT);
				settextstyle(&f);
				settextcolor(WHITE);
				outtextxy(10, 35, _T("等待连接中..."));
				ServerProcess();
				return true;
			}
			else if (operation.x >= 100 && operation.x <= 300 && operation.y <= 280 && operation.y >= 230)
			{
				//这里添加客户端连接代码
				mode = 2;
				ClientProcess(operation);
				return true;
			}
			else if (operation.x >= 100 && operation.x <= 300 && operation.y <= 360 && operation.y >= 310)
			{
				//热座模式进入单机代码
				mode = 3;
				ChessBoard A;
				A.initGraph();
				while (1)
				{
					peekmessage(&operation, -1, 1);//监测鼠标消息
					if (A.game(operation))//退出游戏部分逻辑
						break;
				}
				return true;
			}
		}
		else
			UIUpdate(operation);
		return false;
	}
private:
	IMAGE background;//定义一个图片名.
	void drawButton(int i, int j, int x, int y)
	{
		setfillcolor(BUTTONCOLOR);
		solidroundrect(i, j, x, y, 5, 5);
	}
	void overButton(int i, int j, int x, int y)
	{
		setfillcolor(BUTTONCOLOR + (30, 30, 30));
		solidroundrect(i, j, x, y, 5, 5);
	}
	void UIUpdate(ExMessage& operation)//绘制窗口函数
	{
		BeginBatchDraw();
		FlushBatchDraw();
		//下面是鼠标hover功能的实现
		if (operation.x >= 100 && operation.x <= 300 && operation.y <= 200 && operation.y >= 150)
		{
			overButton(100, 200, 300, 150);
			settextcolor(OVERBUTTON);
			ZhuJi();
		}
		else if (operation.x >= 100 && operation.x <= 300 && operation.y <= 280 && operation.y >= 230)
		{
			overButton(100, 230, 300, 280);
			settextcolor(OVERBUTTON);
			KeHu();
		}
		else if (operation.x >= 100 && operation.x <= 300 && operation.y <= 360 && operation.y >= 310)
		{
			overButton(100, 310, 300, 360);
			settextcolor(OVERBUTTON);
			ReZuo();
		}
		//没有hover的话就绘制回去
		else
		{
			settextcolor(BUTTONTEXT);
			drawButton(100, 200, 300, 150);
			ZhuJi();
			drawButton(100, 230, 300, 280);
			KeHu();
			drawButton(100, 310, 300, 360);
			ReZuo();
		}
		EndBatchDraw();

	}
	void ZhuJi()
	{
		outtextxy(160, 165, _T("我是主机"));
	}
	void KeHu()
	{
		outtextxy(160, 245, _T("我要连接"));
	}
	void ReZuo()
	{
		outtextxy(160, 325, _T("热座模式"));
	}

	string IpGet(ExMessage& operation)//获取IP地址的方法
	{
		// 定义字符串缓冲区，并接收用户输入
		TCHAR s[16];
		InputBox(s, 16, _T("请输入主机的ip地址"));
		return s;
	}

	void ServerProcess()
	{
		ChessSocketServer CServer;
		DrawFisrt();
		ChessBoard A;
		A.initGraph();
		ExMessage operation;
		while (1)
		{
			A.OnlineWinCheck();
			if (isTurn)
			{
				{
					InMessage waitedSignal(0, 0, 0, 0);

					while (1)
					{
						peekmessage(&operation, -1, 1);//监测鼠标消息
						if (operation.message == WM_LBUTTONDOWN)
							waitedSignal = OnlineGame(operation, A);
						if (waitedSignal.x != 0 || waitedSignal.y != 0 || waitedSignal.i != 0 || waitedSignal.j != 0)
						{
							CServer.log << "InMessage send: " << waitedSignal.x << " " << waitedSignal.y << " " << waitedSignal.i << " " << waitedSignal.j << endl;
							break;
						}
					}
					send(CServer.hClient, (const char*)&waitedSignal, sizeof(InMessage), 0);
					A.SelectX = -1;
					A.SelectY = -1;
					isTurn = !isTurn;
				}
			}
			else
			{
				char buf[16];
				recv(CServer.hClient, buf, sizeof(InMessage), 0);
				CServer.log << "PreInMessage recv: " << buf << endl;
				InMessage* reMessage = (InMessage*)&buf;
				CServer.log << "AfterInMessage recv: " << reMessage->x << " " << reMessage->y << " " << reMessage->i << " " << reMessage->j << endl;
				A.movePiece(reMessage->x, reMessage->y, reMessage->i, reMessage->j, operation);
				isTurn = !isTurn;
			}
		}
	}

	void ClientProcess(ExMessage& op)
	{
		ChessSocketClient hSocket(IpGet(op));
		DrawFisrt();
		ChessBoard A;
		A.initGraph();
		ExMessage operation;
		while (1)
		{
			A.OnlineWinCheck();
			if (isTurn)
			{
				{
					InMessage waitedSignal(0, 0, 0, 0);

					while (1)
					{
						peekmessage(&operation, -1, 1);//监测鼠标消息
						if (operation.message == WM_LBUTTONDOWN)
						{
							waitedSignal = OnlineGame(operation, A);
							if (mode == 1)
								Tlog << "waitedSignal: " << waitedSignal.x << " " << waitedSignal.y << " " << waitedSignal.i << " " << waitedSignal.j << " " << A.WasRule << endl;
						}
						if (waitedSignal.x != 0 || waitedSignal.y != 0 || waitedSignal.i != 0 || waitedSignal.j != 0)
						{
							hSocket.log << "InMessage send: " << waitedSignal.x << " " << waitedSignal.y << " " << waitedSignal.i << " " << waitedSignal.j << endl;
							break;
						}
					}
					send(hSocket.hSock, (const char*)&waitedSignal, sizeof(InMessage), 0);
					A.SelectX = -1;
					A.SelectY = -1;
					isTurn = !isTurn;
				}
			}
			else
			{
				char buf[16];
				recv(hSocket.hSock, buf, sizeof(InMessage), 0);
				hSocket.log << "PreInMessage recv: " << buf << endl;
				InMessage* reMessage = (InMessage*)&buf;
				hSocket.log << "AfterInMessage recv: " << reMessage->x << " " << reMessage->y << " " << reMessage->i << " " << reMessage->j << endl;
				A.movePiece(reMessage->x, reMessage->y, reMessage->i, reMessage->j, operation);
				isTurn = !isTurn;
			}
		}
	}

	InMessage OnlineGame(ExMessage& operation, ChessBoard& A)
	{
		InMessage ret(0, 0, 0, 0);
		if (!A.isSelected)
		{
			A.OnlineMessageProcessFirst(operation);
		}
		else
		{
			A.OnlineMessageProcessSecond(operation);
			if (A.WasRule)
			{
				ret.x = A.SocketX;
				ret.y = A.SocketY;
				ret.i = A.SelectX;
				ret.j = A.SelectY;
				if (mode == 1)
					Tlog << ret.x << " " << ret.y << " " << ret.i << " " << ret.j << " " << A.WasRule << endl;
			}
		}
		return ret;
	}
};

int main()
{
	srand((unsigned)time(NULL));
	Tlog.open("Total.log", ios::out | ios::trunc);
	UI UserInterface;
	ExMessage operation;
	while (1)
	{
		peekmessage(&operation, -1, 1);
		if (UserInterface.Process(operation))
			break;
	}
}

void DrawFisrt()
{
	initgraph(400, 200);
	LOGFONT f;
	gettextstyle(&f);						// 获取当前字体设置
	f.lfHeight = 60;						// 设置字体高度为 20
	_tcscpy(f.lfFaceName, _T("黑体"));		// 设置字体为“黑体”(高版本 VC 推荐使用 _tcscpy_s 函数)
	f.lfQuality = ANTIALIASED_QUALITY;		// 设置输出效果为抗锯齿
	setbkmode(TRANSPARENT);
	settextstyle(&f);
	settextcolor(WHITE);
	if (isTurn)
		outtextxy(20, 70, _T("你获得了先手"));
	else
		outtextxy(20, 70, _T("你获得了后手"));
	Sleep(1500);
}
