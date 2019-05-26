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
	NONE
};

vector<vector<char>> gamemap;
vector<vector<char>> objectmap;

string res[]={"","■", "▧", "☆", "", ""};

string getresource(int type)
{
	return res[type-'0'];
}

void gameinit()
{
	setlocale(LC_ALL,""); 

	resize_term(GAMEY, GAMEX);
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
			mvprintw(i,2*j,"%s",getresource(objectmap[i][j]).c_str());
		}
	}
	refresh();
}

int main()
{
	gameinit();
	loadstage(2);	// 1 ~ 5
	refreshmap();
	getch();
	endwin();
	return 0;
}
