#include <ncurses.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define GAMEY 25
#define GAMEX 40

using namespace std;

/*
☆ ★ ○ ● ◎ ◇ ◆ □ ■ △ ▲ ▽ ▼ → ← ↑ ↓ ↔ 〓 ◁ ◀ ▷ ▶ ♤ ♠ ♡ ♥ ♧ ♣ ⊙ ◈ ▣ ◐ ◑ ▒ ▤ ▥ ▨ ▧ ▦
▩
*/

// 블럭 타입 열거체
enum BLOCK { FLOOR = '0', WALL, BOX, GOAL, AIR, PLAYER, NONE };

int level;         // 현재 스테이지 레벨
int step;          // 캐릭터가 움직인 횟수
int prev_step;     // Undo
int push;          // 상자가 움직인 횟수
int prev_push;     // Undo
int playery;       // 플레이어의 y 좌표
int prev_playery;  // Undo
int playerx;       // 플레이어의 x 좌표
int prev_playerx;  // Undo
int undo_check;
int diry[4] = {1, -1, 0, 0};  // 방향에 따른 y 좌표 변환 배열
int dirx[4] = {0, 0, -1, 1};  // 뱡향에 따른 x 좌표 변환 배열
vector<vector<char>> gamemap;  // 맵 데이터(벽, 목표지점, 바닥, 맵밖)
vector<vector<char>> prev_gamemap;  // Undo
vector<vector<char>> objectmap;  // 오브젝트 데이터(상자, 플레이어)
vector<vector<char>> prev_objectmap;  // Undo

string res[] = {"  ", "██", "⍈⍇", "◂▸", "  ", "홋", ""};  // 블럭 리소스

WINDOW *right_top;     // 오른쪽 점수판
WINDOW *right_bottom;  // 오른쪽 플레이 안내
WINDOW *title;         // 상단 타이틀
WINDOW *game;          // 게임 화면

// 블럭 타입을 리소스로 변환
string getresource(int type) {
    // type을 인덱스로 변환해서 반환
    return res[type - FLOOR];
}

// nCurses의 키를 입력받으면 변환해야할 y 좌표 값을 반환
int getdiry(int dir) { return diry[dir - KEY_DOWN]; }

// nCurses의 키를 입력받으면 변환해야할 x 좌표 값을 반환
int getdirx(int dir) { return dirx[dir - KEY_DOWN]; }

// ncurses 초기화
void ncursesinit() {
    // 한글 출력을 위한 locale설정
    setlocale(LC_ALL, "");

    char cmd[100];
    sprintf(cmd, "resize -s %d %d", GAMEY, 2 * GAMEX);
    system(cmd);

    // curses 모드 시작
    initscr();
    start_color();

    // 색상 추가
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    // 맵 크기에 맞게 터미널 크기 변경
    resize_term(GAMEY, 2 * GAMEX);
    // 키보드 입력
    keypad(stdscr, TRUE);

    // 커서설정
    curs_set(0);
    noecho();

    // 터미널 전체 보더
    border(ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER,
           ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
    refresh();
}

// 게임 첫 실행 화면
void welcome() {
    char logo[] =
        "███████╗ ██████╗ ██╗  ██╗ ██████╗ ██████╗  █████╗ ███╗   "
        "██╗\n██╔════╝██╔═══██╗██║ ██╔╝██╔═══██╗██╔══██╗██╔══██╗████╗  "
        "██║\n███████╗██║   ██║█████╔╝ ██║   ██║██████╔╝███████║██╔██╗ "
        "██║\n╚════██║██║   ██║██╔═██╗ ██║   "
        "██║██╔══██╗██╔══██║██║╚██╗██║\n███████║╚██████╔╝██║  "
        "██╗╚██████╔╝██████╔╝██║  ██║██║ ╚████║\n╚══════╝ ╚═════╝ ╚═╝  ╚═╝ "
        "╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝";
    char text[] = "Press any key to start";

    // 게임 첫 화면 레이아웃 윈도우
    WINDOW *welcome_logo = newwin(5, 69, 7, 10);
    WINDOW *desc = newwin(1, 22, 18, 29);
    // 로고 및 설명 출력
    wprintw(welcome_logo, logo);
    wattron(desc, COLOR_PAIR(1));
    wprintw(desc, text);
    wattroff(desc, COLOR_PAIR(1));
    // 화면 갱신
    wrefresh(welcome_logo);
    wrefresh(desc);
    // 아무키나 눌러 시작
    getch();
    // 새 레이아웃 제거
    delwin(welcome_logo);
    delwin(desc);
}

// 게임 초기설정
void gameinit() {
    // 터미널 전체 보더
    border(ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER,
           ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
    refresh();

    // 윈도우 선언
    right_top = newwin(13, 30, 2, 50);
    right_bottom = newwin(11, 30, 14, 50);
    title = newwin(3, 0, 0, 0);
    game = newwin(21, 49, 3, 1);

    // 윈도우 내부 보더 표시
    wborder(right_top, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_TTEE,
            ACS_RTEE, ACS_LTEE, ACS_RTEE);
    wborder(right_bottom, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_LTEE,
            ACS_RTEE, ACS_BTEE, ACS_LRCORNER);
    wborder(title, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER,
            ACS_URCORNER, ACS_LTEE, ACS_RTEE);
    wborder(game, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER,
            ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);

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

    wborder(right_top, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_TTEE,
            ACS_RTEE, ACS_LTEE, ACS_RTEE);

    undo_check = prev_push = prev_step = step = push = 0;
    if (f.is_open()) {
        int r, c;
        f >> r >> c;
        gamemap.resize(r, vector<char>(c, FLOOR));
        objectmap.resize(r, vector<char>(c, NONE));
        for (int i = 0; i < r; i++) {
            for (int j = 0; j < c; j++) {
                char ch;
                f >> ch;
                if (ch == BOX)
                    objectmap[i][j] = ch;
                else
                    gamemap[i][j] = ch;
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

// 스테이지 클리어 화면
void completestage(int level) {
    // 스테이지 클리어 윈도우
    WINDOW *complete = newwin(23, 0, 2, 0);
    wborder(complete, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_LTEE,
            ACS_RTEE, ACS_LLCORNER, ACS_LRCORNER);
    // 스테이지 번호 아스키 아트
    char number[][500] = {
        "",
        "    ▄▄▄▄     \n  ▄█░░░░▌    \n ▐░░▌▐░░▌    \n  ▀▀ ▐░░▌    \n     ▐░░▌ "
        "   \n     ▐░░▌    \n     ▐░░▌    \n     ▐░░▌    \n ▄▄▄▄█░░█▄▄▄ "
        "\n▐░░░░░░░░░░░▌\n ▀▀▀▀▀▀▀▀▀▀▀ ",
        " ▄▄▄▄▄▄▄▄▄▄▄ \n▐░░░░░░░░░░░▌\n ▀▀▀▀▀▀▀▀▀█░▌\n          ▐░▌\n          "
        "▐░▌\n ▄▄▄▄▄▄▄▄▄█░▌\n▐░░░░░░░░░░░▌\n▐░█▀▀▀▀▀▀▀▀▀ \n▐░█▄▄▄▄▄▄▄▄▄ "
        "\n▐░░░░░░░░░░░▌\n ▀▀▀▀▀▀▀▀▀▀▀ ",
        " ▄▄▄▄▄▄▄▄▄▄▄ \n▐░░░░░░░░░░░▌\n ▀▀▀▀▀▀▀▀▀█░▌\n          ▐░▌\n "
        "▄▄▄▄▄▄▄▄▄█░▌\n▐░░░░░░░░░░░▌\n ▀▀▀▀▀▀▀▀▀█░▌\n          ▐░▌\n "
        "▄▄▄▄▄▄▄▄▄█░▌\n▐░░░░░░░░░░░▌\n ▀▀▀▀▀▀▀▀▀▀▀ ",
        " ▄         ▄ \n▐░▌       ▐░▌\n▐░▌       ▐░▌\n▐░▌       "
        "▐░▌\n▐░█▄▄▄▄▄▄▄█░▌\n▐░░░░░░░░░░░▌\n ▀▀▀▀▀▀▀▀▀█░▌\n          ▐░▌\n     "
        "     ▐░▌\n          ▐░▌\n           ▀ ",
        " ▄▄▄▄▄▄▄▄▄▄▄ \n▐░░░░░░░░░░░▌\n▐░█▀▀▀▀▀▀▀▀▀ \n▐░█▄▄▄▄▄▄▄▄▄ "
        "\n▐░░░░░░░░░░░▌\n ▀▀▀▀▀▀▀▀▀█░▌\n          ▐░▌\n          ▐░▌\n "
        "▄▄▄▄▄▄▄▄▄█░▌\n▐░░░░░░░░░░░▌\n ▀▀▀▀▀▀▀▀▀▀▀ "};
    WINDOW *stage_num = newwin(11, 14, 7, 33);
    // 스테이지 클리어 문구 출력
    wattron(complete, COLOR_PAIR(1));
    mvwprintw(complete, 2, 33, "Stage Complete", level);
    mvwprintw(complete, 18, 26, "Press any key to continue...");
    wattroff(complete, COLOR_PAIR(1));
    // 스테이지 번호 출력
    wprintw(stage_num, number[level]);
    // 화면 출력
    wrefresh(complete);
    wrefresh(stage_num);
    getch();
    // 윈도우 제거
    delwin(stage_num);
    delwin(complete);
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
            mvwprintw(game, i + offsety, 2 * j + offsetx, "%s",
                      getresource(gamemap[i][j]).c_str());
        }
    }
    // 그 위에 오브젝트 데이터 출력
    for (int i = 0; i < objectmap.size(); i++) {
        for (int j = 0; j < objectmap[i].size(); j++) {
            mvwprintw(game, i + offsety, 2 * j + offsetx, "%s",
                      getresource(objectmap[i][j]).c_str());
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
    mvwprintw(right_top, 9, 8, "Undo  :  %s",
              (!undo_check) ? "Enabled " : "Disabled");
    wrefresh(right_top);
}

// nCurses의 키를 입력받으면, 오브젝트의 움직임을 처리
bool moveobject(int y, int x, int dir, int count) {
    if (count > 2) return false;
    int movey = y + getdiry(dir), movex = x + getdirx(dir);
    // 이동하려는 방향에 벽인지 확인
    if (gamemap[movey][movex] == WALL) return false;
    // 이동하려는 방향이 상자인지 확인
    if (objectmap[movey][movex] == BOX) {
        // 상자를 움직일 수 있는지 확인
        if (!moveobject(movey, movex, dir, count + 1)) return false;
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
            if (gamemap[y][x] == GOAL && objectmap[y][x] != BOX) return false;
    return true;
}

// 실행취소
void undo() {
    // 움직인 적이 있을때만 사용이 가능
    if (!step) return;
    // 현재 스테이지에서 실행 취소를 한 적이 있으면 return
    if (undo_check) return;
    // 이전 데이터로 롤백
    step = prev_step;
    push = prev_push;
    playerx = prev_playerx;
    playery = prev_playery;
    gamemap = prev_gamemap;
    objectmap = prev_objectmap;
    undo_check = 1;
}

int keyevent() {
    int key;
    vector<vector<char>> temp_objectmap;
    vector<vector<char>> temp_gamemap;
    int temp_playerx;
    int temp_playery;
    int temp_step;
    int temp_push;
    do {
        // movement
        switch (key) {
            case KEY_DOWN:
            case KEY_UP:
            case KEY_LEFT:
            case KEY_RIGHT:
                temp_objectmap = objectmap;
                temp_gamemap = gamemap;
                temp_playerx = playerx;
                temp_playery = playery;
                temp_step = step;
                temp_push = push;
                if (moveobject(playery, playerx, key, 1)) {
                    prev_gamemap = temp_gamemap;
                    prev_objectmap = temp_objectmap;
                    prev_playerx = temp_playerx;
                    prev_playery = temp_playery;
                    prev_step = temp_step;
                    prev_push = temp_push;
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
    // ncourses 초기화
    ncursesinit();
    // welcome 화면
    welcome();
    for (level = 1; level <= 5; level++) {
        // 게임 초기설정
        gameinit();
        // 스테이지 로드
        loadstage(level);       // 1 ~ 5
        if (keyevent()) break;  // q키를 입력하면 게임 종료
        completestage(level);   // 스테이지 클리어 화면
    }
    endwin();
    return 0;
}
