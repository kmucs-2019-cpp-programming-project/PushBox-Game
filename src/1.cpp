#include <ncurses.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>

#define GAMEY 25
#define GAMEX 40

using namespace std;

/*
☆ ★ ○ ● ◎ ◇ ◆ □ ■ △ ▲ ▽ ▼ → ← ↑ ↓ ↔ 〓 ◁ ◀ ▷ ▶ ♤ ♠ ♡ ♥ ♧ ♣ ⊙ ◈ ▣ ◐ ◑ ▒ ▤ ▥ ▨ ▧ ▦ ▩ 
*/

// 블럭 타입 열거체
enum BLOCK{
	FLOOR='0',
	WALL,
	BOX,
	GOAL,
	AIR,
	PLAYER,
	NONE
};

int step; // 캐릭터가 움직인 횟수
int push; // 상자가 움직인 횟수
int playery; // 플레이어의 y 좌표
int playerx; // 플레이어의 x 좌표
int diry[4] = {1, -1, 0, 0}; // 방향에 따른 y 좌표 변환 배열
int dirx[4] = {0, 0, -1, 1}; // 뱡향에 따른 x 좌표 변환 배열
vector<vector<char>> gamemap;	// 맵 데이터(벽, 목표지점, 바닥, 맵밖)
vector<vector<char>> objectmap;	// 오브젝트 데이터(상자, 플레이어)

string res[]={" ","■", "▧", "☆", " ", "●", ""};	// 블럭 리소스

// 블럭 타입을 리소스로 변환
string getresource(int type)
{
	// type을 인덱스로 변환해서 반환
	return res[type - FLOOR];
}

// nCurses의 키를 입력받으면 변환해야할 y 좌표 값을 반환
int getdiry(int dir)
{
	return diry[dir - KEY_DOWN];
}

// nCurses의 키를 입력받으면 변환해야할 x 좌표 값을 반환
int getdirx(int dir)
{
	return dirx[dir - KEY_DOWN];
}

// 게임(ncurses) 초기설정
void gameinit()
{
	// 한글 출력을 위한 locale설정
	setlocale(LC_ALL,""); 

	// 맵 크기에 맞게 터미널 크기 변경
	resize_term(GAMEY, 2*GAMEX);
	char cmd[100];
	sprintf(cmd, "resize -s %d %d", GAMEY, 2*GAMEX);
	system(cmd);

	// curses 모드 시작
	initscr();
}

// 스테이지 로드(파일 입출력)
void loadstage(int stage_num)
{
	// 스테이지 데이터 파일 열기
	ifstream f("stage/" + to_string(stage_num));
	string s;
	if (f.is_open()){
		while (getline(f, s)){
			vector<char> g, o;
			for(char ch : s){
				if(ch==' ') continue;
				if(ch==BOX){
					g.push_back(FLOOR);
					o.push_back(ch);
				}
				else{
					g.push_back(ch);
					o.push_back(NONE);
				}
			}
			gamemap.push_back(g);
			objectmap.push_back(o);
		}
		playery = playerx = 2;
		objectmap[playery][playerx] = PLAYER;
		f.close();
	}
	else{
		printw("Not Found File");
	}
}

// 화면 업데이트
void refreshmap()
{
	// 맵 데이터 출력
	for(int i=0; i<gamemap.size(); i++){
		for(int j=0; j<gamemap[i].size(); j++){
			mvprintw(i,2*j,"%s",getresource(gamemap[i][j]).c_str());
		}
	}
	// 그 위에 오브젝트 데이터 출력
	for(int i=0; i<objectmap.size(); i++){
		for(int j=0; j<objectmap[i].size(); j++){
			mvprintw(i,2*j,"%s", getresource(objectmap[i][j]).c_str());
		}
	}
	// 터미널 화면 업데이트
	refresh();
}

// nCurses의 키를 입력받으면, 오브젝트의 움직임을 처리
bool moveobject(int y, int x, int dir, int count)
{
	if(count > 2) return false;
	int movey = y + getdiry(dir), movex = x + getdirx(dir);
	if(gamemap[movey][movex] == WALL)
		return false;
	if(objectmap[movey][movex] == BOX){
		if(!moveobject(movey, movex, dir, count + 1))
			return false;
		push++;
		step--;
	}
	objectmap[movey][movex] = objectmap[y][x];
	objectmap[y][x] = NONE;
	step++;
	return true;
}

// 스테이지가 클리어 되었는지 확인
bool clearcheck()
{
	for(int y = 0; y < gamemap.size(); y++)
		for(int x = 0; x < gamemap[y].size(); x++)
			if(gamemap[y][x] == GOAL && objectmap[y][x] != BOX)
				return false;
	return true;
}

void keyevent(){
	int key;
	while((key = getch()) != KEY_F(2)){
		// movement
		switch (key)
		{
		case KEY_DOWN:
		case KEY_UP:
		case KEY_LEFT:
		case KEY_RIGHT:
			if(moveobject(playery, playerx, key, 1)){
				playery += getdiry(key);
				playerx += getdirx(key);
			}
			clearcheck();
			break;
		
		default:
			break;
		}

		refreshmap();
	}
}

// 메인함수
int main()
{
	// 게임 초기설정
	gameinit();
	// 스테이지 로드
	loadstage(2);	// 1 ~ 5
	keyevent();
	endwin();
	return 0;
}
