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

enum BLOCK{
	FLOOR='0',
	WALL,
	BOX,
	GOAL,
	AIR,
	PLAYER,
	NONE
};

int playery, playerx;
int dirx[4] = {0, 0, -1, 1};
int diry[4] = {1, -1, 0, 0};
vector<vector<char>> gamemap;
vector<vector<char>> objectmap;

string res[]={" ","■", "▧", "☆", " ", "●", ""};

string getresource(int type)
{
	return res[type - FLOOR];
}

int getdiry(int dir)
{
	return diry[dir - KEY_DOWN];
}

int getdirx(int dir)
{
	return dirx[dir - KEY_DOWN];
}

void gameinit()
{
	setlocale(LC_ALL,""); 

	resize_term(GAMEY, 2*GAMEX);
	char cmd[100];
	sprintf(cmd, "resize -s %d %d", GAMEY, 2*GAMEX);
	system(cmd);

	initscr();

	keypad(stdscr, true);
	curs_set(0);
	noecho();
}

void loadstage(int stage_num)
{
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

void refreshmap()
{
	for(int i=0; i<gamemap.size(); i++){
		for(int j=0; j<gamemap[i].size(); j++){
			mvprintw(i,2*j,"%s",getresource(gamemap[i][j]).c_str());
		}
	}
	for(int i=0; i<objectmap.size(); i++){
		for(int j=0; j<objectmap[i].size(); j++){
			mvprintw(i,2*j,"%s", getresource(objectmap[i][j]).c_str());
		}
	}
	refresh();
}

bool moveobject(int y, int x, int dir, int step)
{
	if(step > 2) return false;
	int movey = y + getdiry(dir), movex = x + getdirx(dir);
	if(gamemap[movey][movex] == WALL)
		return false;
	if(objectmap[movey][movex] == BOX && !moveobject(movey, movex, dir, step + 1))
		return false;
	objectmap[movey][movex] = objectmap[y][x];
	objectmap[y][x] = NONE;
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
			break;
		
		default:
			break;
		}

		refreshmap();
	}
}

int main()
{
	gameinit();
	loadstage(2);	// 1 ~ 5
	keyevent();
	endwin();
	return 0;
}
