#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <vector>

#define GAMEY 25
#define GAMEX 40

using namespace std;

/*
☆ ★ ○ ● ◎ ◇ ◆ □ ■ △ ▲ ▽ ▼ → ← ↑ ↓ ↔ 〓 ◁ ◀ ▷ ▶ ♤ ♠ ♡ ♥ ♧ ♣ ⊙ ◈ ▣ ◐ ◑ ▒ ▤ ▥ ▨ ▧ ▦ ▩ 
*/

// 블럭 타입 열거체
enum BLOCK { FLOOR = '0',
             WALL,
             BOX,
             GOAL,
             AIR,
             PLAYER,
             NONE };

int level;                      // 현재 스테이지 레벨
int step;                       // 캐릭터가 움직인 횟수
int prev_step;                  // Undo
int push;                       // 상자가 움직인 횟수
int prev_push;                  // Undo
int playery;                    // 플레이어의 y 좌표
int prev_playery;               // Undo
int playerx;                    // 플레이어의 x 좌표
int prev_playerx;               // Undo
int diry[4] = {1, -1, 0, 0};    // 방향에 따른 y 좌표 변환 배열
int dirx[4] = {0, 0, -1, 1};    // 뱡향에 따른 x 좌표 변환 배열
vector<vector<char>> gamemap;   // 맵 데이터(벽, 목표지점, 바닥, 맵밖)
vector<vector<char>> prev_gamemap;  // Undo
vector<vector<char>> objectmap; // 오브젝트 데이터(상자, 플레이어)
vector<vector<char>> prev_objectmap;    // Undo

string res[] = {"  ", "██", "⍈⍇", "◂▸", "  ", "홋", ""}; // 블럭 리소스

WINDOW *right_top;    // 오른쪽 점수판
WINDOW *right_bottom; // 오른쪽 플레이 안내
WINDOW *title;       // 상단 타이틀
WINDOW *game;        // 게임 화면

// 블럭 타입을 리소스로 변환
string getresource(int type) {
    // type을 인덱스로 변환해서 반환
    return res[type - FLOOR];
}

// nCurses의 키를 입력받으면 변환해야할 y 좌표 값을 반환
int getdiry(int dir) { return diry[dir - KEY_DOWN]; }

// nCurses의 키를 입력받으면 변환해야할 x 좌표 값을 반환
int getdirx(int dir) { return dirx[dir - KEY_DOWN]; }

// 게임(ncurses) 초기설정
void gameinit() {
    // 한글 출력을 위한 locale설정
    setlocale(LC_ALL, "");

    char cmd[100];
    sprintf(cmd, "resize -s %d %d", GAMEY, 2 * GAMEX);
    system(cmd);

    // curses 모드 시작
    initscr();
    start_color();

    // 맵 크기에 맞게 터미널 크기 변경

    resize_term(GAMEY, 2 * GAMEX);
    // 키보드 입력
    keypad(stdscr, TRUE);

    // 커서설정
    curs_set(0);
    noecho();

    // 터미널 전체 보더
    border(ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
    refresh();

    // 윈도우 선언
    right_top = newwin(13, 30, 2, 50);
    right_bottom = newwin(11, 30, 14, 50);
    title = newwin(3, 0, 0, 0);
    game = newwin(21, 47, 3, 2);

    // 윈도우 내부 보더 표시
    wborder(right_top, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_TTEE, ACS_RTEE, ACS_LTEE, ACS_RTEE);
    wborder(right_bottom, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_LTEE, ACS_RTEE, ACS_BTEE, ACS_LRCORNER);
    wborder(title, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LTEE, ACS_RTEE);
    wborder(game, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);

    // 타이틀에 글자 표시
    mvwprintw(title, 1, 33, "S O K O B A N");

    // 설명 입력
    mvwprintw(right_bottom, 1, 10, "↑");
    mvwprintw(right_bottom, 2, 8, "← ● → : Move");
    mvwprintw(right_bottom, 3, 10, "↓");
    mvwprintw(right_bottom, 5, 10, "r   : Reset");
    mvwprintw(right_bottom, 7, 10, "q   : Quit");
    mvwprintw(right_bottom, 9, 10, "z   : Undo");

    // 윈도우 갱신
    wrefresh(title);
    wrefresh(right_top);
    wrefresh(right_bottom);
    wrefresh(game);
}

// 스테이지 로드(파일 입출력)
void loadstage(int stage_num) {
    // 스테이지 데이터 파일 열기
    ifstream f("stage/" + to_string(stage_num));
    string s;
    gamemap.clear();
    objectmap.clear();

    wclear(game);
    wclear(right_top);

    wborder(right_top, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_TTEE, ACS_RTEE, ACS_LTEE, ACS_RTEE);

    prev_push = prev_step = step = push = 0;
    if (f.is_open()) {
        int r, c;
        f >> r >> c;
        gamemap.resize(r, vector<char>(c, FLOOR));
        objectmap.resize(r, vector<char>(c, NONE));
        for(int i = 0; i < r; i++){
            for(int j = 0; j < c; j++){
                char ch;
                f >> ch;
                if(ch == BOX) objectmap[i][j] = ch;
                else gamemap[i][j] = ch;
            }
        }
        f >> playery >> playerx;
        objectmap[playery][playerx] = PLAYER;
        prev_gamemap = gamemap;
        prev_objectmap = objectmap;
        prev_playerx = playerx;
        prev_playery = playery;
        f.close();
    } else {
        printw("파일 없음");
    }
}

// 화면 업데이트
void refreshmap() {
    // 맵을 윈도우 중앙에 놓기
    int offsety, offsetx;
    offsety = 10 - gamemap.size() / 2;
    offsetx = 24 - gamemap[0].size();
    // 맵 데이터 출력
    for (int i = 0; i < gamemap.size(); i++) {
        for (int j = 0; j < gamemap[i].size(); j++) {
            mvwprintw(game, i + offsety, 2 * j + offsetx, "%s", getresource(gamemap[i][j]).c_str());
        }
    }
    // 그 위에 오브젝트 데이터 출력
    for (int i = 0; i < objectmap.size(); i++) {
        for (int j = 0; j < objectmap[i].size(); j++) {
            mvwprintw(game, i + offsety, 2 * j + offsetx, "%s", getresource(objectmap[i][j]).c_str());
        }
    }
    // 터미널 화면 업데이트
    wrefresh(game);
}

// 우측 정보 업데이트
void refreshstatus() {
    mvwprintw(right_top, 3, 8, "Level :  %d", level);
    mvwprintw(right_top, 5, 8, "Step  :  %d", step);
    mvwprintw(right_top, 7, 8, "Push  :  %d", push);
    mvwprintw(right_top, 9, 8, "Undo  :  %s", (prev_push != -1) ? "Able" : "Disabled");
    wrefresh(right_top);
}

// nCurses의 키를 입력받으면, 오브젝트의 움직임을 처리
bool moveobject(int y, int x, int dir, int count) {
    if (count > 2)
        return false;
    int movey = y + getdiry(dir), movex = x + getdirx(dir);
    // 이동하려는 방향에 벽인지 확인
    if (gamemap[movey][movex] == WALL)
        return false;
    // 이동하려는 방향이 상자인지 확인
    if (objectmap[movey][movex] == BOX) {
        // 상자를 움직일 수 있는지 확인
        if (!moveobject(movey, movex, dir, count + 1))
            return false;
        // 상자를 움직인 횟수 증가
        push++;
        // 플레이어가 움직인 횟수 감소
        step--;
    }
    // 현재 물체를 이동하려는 방향으로 옮김
    objectmap[movey][movex] = objectmap[y][x];
    objectmap[y][x] = NONE;
    // 플레이어가 움직인 횟수 증가
    step++;
    return true;
}

// 스테이지가 클리어 되었는지 확인
bool clearcheck() {
    for (int y = 0; y < gamemap.size(); y++)
        for (int x = 0; x < gamemap[y].size(); x++)
            if (gamemap[y][x] == GOAL && objectmap[y][x] != BOX)
                return false;
    return true;
}

// 실행취소
void undo() {
    // 움직인 적이 있을때만 사용이 가능
    if(!step) return;
    // 이전에 실행취소했다면 return
    if(prev_push == -1) return;
    // 이전 데이터로 롤백
    step = prev_step;
    push = prev_push;
    playerx = prev_playerx;
    playery = prev_playery;
    gamemap = prev_gamemap;
    objectmap = prev_objectmap;
    prev_push = -1;
}

int keyevent() {
    int key;
    do {
        // movement
        switch (key) {
        case KEY_DOWN:
        case KEY_UP:
        case KEY_LEFT:
        case KEY_RIGHT:
            prev_gamemap = gamemap;
            prev_objectmap = objectmap;
            prev_playerx = playerx;
            prev_playery = playery;
            prev_step = step;
            prev_push = push;
            if (moveobject(playery, playerx, key, 1)) {
                playery += getdiry(key);
                playerx += getdirx(key);
            }
            break;

        case 'q':
            return 1;

        case 'r':
            loadstage(level);
            break;

        case 'z':
            undo();
            break;

        default:
            break;
        }

        refreshmap();
        refreshstatus();
        // 스테이지가 클리어 되었는지 확인한다.
        if (clearcheck()) {
            return 0;
        }
    } while ((key = getch()) != KEY_F(2));
    return 1;
    // F2키를 누르면 게임을 즉시 중단한다.
}

// 메인함수
int main() {
    // 게임 초기설정
    gameinit();
    // 스테이지 로드
    for (level = 1; level <= 5; level++) {
        loadstage(level); // 1 ~ 5
        if (keyevent())
            break;
    }
    endwin();
    return 0;
}
