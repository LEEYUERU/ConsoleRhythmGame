#include <iostream>
#include <vector>
#include <ctime>
#include <algorithm> // for std::remove_if
#include <string>    // for std::to_string

#ifdef _WIN32
#include <windows.h> // For SetConsoleOutputCP
#endif
#include "rlutil/rlutil.h"

using namespace std;

// 遊戲參數設定
const int JUDGE_LINE = 35;     // 判定線位置
const int TRACKS[4] = {20, 40, 60, 80}; // 四個軌道的 X 座標
const char KEYS[4] = {'d', 'f', 'j', 'k'}; // 對應按鍵

struct Note {
    int x, y;
    bool active;
    // Overload == for easier comparison
    bool operator==(const Note& other) const {
        return x == other.x && y == other.y && active == other.active;
    }
};

// --- 全域遊戲狀態 ---
int score = 0;
int combo = 0;
int hp = 100;
vector<Note> notes;
vector<Note> last_notes; // 用於清除上一幀的音符

// --- UI 更新相關狀態 ---
int flashTrack = -1;
int flashDuration = 0;
int last_score = -1, last_combo = -1, last_hp = -1;
int last_track_color = -1;

// 根據分數獲取軌道顏色
int getTrackColor(int current_score) {
    if (current_score < 2000) return rlutil::WHITE;
    if (current_score < 4000) return rlutil::LIGHTRED;
    if (current_score < 6000) return rlutil::YELLOW;
    if (current_score < 8000) return rlutil::LIGHTGREEN;
    if (current_score < 10000) return rlutil::LIGHTCYAN;
    if (current_score < 12000) return rlutil::LIGHTBLUE;
    return rlutil::LIGHTMAGENTA; // 分數超過 12000
}

// 繪製軌道的一部分（用於擦除或重繪）
void drawTrackSegment(int x, int y) {
    rlutil::locate(x, y);
    int trackColor = getTrackColor(score);
    rlutil::setColor(trackColor);
    if (y == JUDGE_LINE) {
        // 如果該軌道正在閃爍，則改變顏色
        if (flashDuration > 0 && x == TRACKS[flashTrack]) {
             rlutil::setColor(rlutil::WHITE);
        } else {
             rlutil::setColor(rlutil::CYAN);
        }
        cout << "====";
    } else {
        cout << "|  |";
    }
}

// 繪製靜態遊戲框架
void drawFrame() {
    int trackColor = getTrackColor(score);
    last_track_color = trackColor;
    rlutil::setColor(trackColor);

    // 繪製軌道
    for (int i = 0; i < 4; i++) {
        for (int j = 1; j <= JUDGE_LINE; j++) {
            drawTrackSegment(TRACKS[i], j);
        }
    }

    // 繪製靜態資訊標籤
    rlutil::setColor(rlutil::WHITE);
    rlutil::locate(90, 5); cout << "Score: ";
    rlutil::locate(90, 6); cout << "Combo: ";
    rlutil::locate(90, 7); cout << "HP: ";
}

// 只更新變化的 UI 元素
void updateDynamicUI() {
    // 檢查軌道顏色是否需要更新
    int current_track_color = getTrackColor(score);
    if (current_track_color != last_track_color) {
        drawFrame(); // 分數跨越門檻，重繪整個框架
        // 重繪當前所有音符，因為背景變了
        for(const auto& n : notes) {
            if (!n.active) continue;
            rlutil::locate(n.x, n.y);
            rlutil::setColor(rlutil::YELLOW);
            cout << "[##]";
        }
    }

    // 更新閃爍的判定線
    if (flashDuration > 0) {
        flashDuration--;
        // 畫白色閃爍
        drawTrackSegment(TRACKS[flashTrack], JUDGE_LINE);
    } else if (flashTrack != -1) {
        // 恢復閃爍軌道的正常顏色
        drawTrackSegment(TRACKS[flashTrack], JUDGE_LINE);
        flashTrack = -1; // 重置
    }

    // 只在數值變化時更新
    if (score != last_score) {
        rlutil::locate(90 + 7, 5);
        rlutil::setColor(rlutil::WHITE);
        cout << score << " "; // 加一個空格來覆蓋舊的長數字
        last_score = score;
    }
    if (combo != last_combo) {
        rlutil::locate(90 + 7, 6);
        rlutil::setColor(rlutil::WHITE);
        cout << combo << " ";
        last_combo = combo;
    }
    if (hp != last_hp) {
        rlutil::locate(90 + 4, 7);
        rlutil::setColor(rlutil::WHITE);
        cout << hp << "   "; // 多加空格確保清除
        last_hp = hp;
    }
}


// 處理判定邏輯
void checkInput(char key) {
    int trackIdx = -1;
    for(int i = 0; i < 4; i++) {
        if(KEYS[i] == key) {
            trackIdx = i;
            break;
        }
    }
    if (trackIdx == -1) return; // 按下的不是遊戲鍵

    Note* hitNote = nullptr;
    int min_dist = 2; // 只處理判定線上下 1 格內的音符

    for (auto &n : notes) {
        if (n.active && n.x == TRACKS[trackIdx]) {
            int dist = abs(n.y - JUDGE_LINE);
            if (dist < min_dist) {
                min_dist = dist;
                hitNote = &n;
            }
        }
    }

    if (hitNote) {
        if (min_dist == 0) { // Perfect
            score += 100;
            combo++;
            flashTrack = trackIdx;
            flashDuration = 2;
        } else if (min_dist == 1) { // Good
            score += 50;
            combo++;
        }
        // 無論 Perfect 還是 Good，都將音符設為非活動
        hitNote->active = false;
        // 從螢幕上擦除音符
        drawTrackSegment(hitNote->x, hitNote->y);
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    rlutil::saveDefaultColor();
    rlutil::hidecursor();
    srand(time(NULL));

    // --- 遊戲主選單 (這部分仍然使用 cls) ---
    int selectedOption = 0;
    const int numOptions = 2;
    auto drawMenu = [&](int selected) {
        rlutil::cls();
        rlutil::locate(10, 5); cout << "--- Rhythm Master (rlutil Edition) ---";
        rlutil::locate(15, 8);
        if (selected == 0) rlutil::setColor(rlutil::YELLOW);
        cout << "開始遊戲";
        rlutil::setColor(rlutil::WHITE);
        rlutil::locate(15, 10);
        if (selected == 1) rlutil::setColor(rlutil::YELLOW);
        cout << "結束遊戲";
        rlutil::setColor(rlutil::WHITE);
        rlutil::locate(10, 15); cout << "使用上下箭頭選擇，按下 Enter 確認";
    };

    while (true) {
        drawMenu(selectedOption);
        int key = rlutil::getkey();
        if (key == rlutil::KEY_UP) selectedOption = (selectedOption - 1 + numOptions) % numOptions;
        else if (key == rlutil::KEY_DOWN) selectedOption = (selectedOption + 1) % numOptions;
        else if (key == rlutil::KEY_ENTER) {
            if (selectedOption == 0) break;
            if (selectedOption == 1) {
                rlutil::cls();
                rlutil::showcursor();
                return 0;
            }
        }
    }
    
    // --- 遊戲開始 ---
    rlutil::setBackgroundColor(rlutil::BLACK);
    rlutil::cls(); // 最後一次清屏
    drawFrame(); // 繪製靜態框架

    while (hp > 0) {
        // 儲存當前幀的狀態，用於下一幀的擦除
        last_notes = notes;

        // --- 擦除上一幀的音符 ---
        for (const auto& n : last_notes) {
            if (n.active) { // 只擦除仍然存在的音符
                drawTrackSegment(n.x, n.y);
            }
        }

        // --- 遊戲邏輯更新 ---
        // 隨機生成音符
        if (rand() % 10 < 2) {
            notes.push_back({TRACKS[rand() % 4], 1, true});
        }

        // 處理輸入
        char k = rlutil::nb_getch();
        if (k != 0) {
            if (k == 'q') break;
            checkInput(k);
        }

        // 更新音符位置與判定 Miss
        for (auto &n : notes) {
            if (!n.active) continue;
            n.y++; // 音符下落
            if (n.y > JUDGE_LINE + 1) { // Miss 判定
                n.active = false;
                hp -= 10;
                combo = 0;
                // No need to draw here, the erase loop handles it.
            }
        }
        
        // --- 重繪 ---
        // 清除不再活動的音符 (從 vector 中)
        notes.erase(
            std::remove_if(notes.begin(), notes.end(), [](const Note& n) {
                return !n.active;
            }),
            notes.end()
        );

        // 繪製當前所有活動的音符
        for (const auto& n : notes) {
            if (!n.active) continue;
            rlutil::locate(n.x, n.y);
            rlutil::setColor(rlutil::YELLOW);
            cout << "[##]";
        }
        
        // 更新 UI 上的動態訊息 (分數, HP 等)
        updateDynamicUI();

        // 根據難度調整休眠時間
        int sleep_ms = 250 - (score / 200);
        if (sleep_ms < 20) sleep_ms = 20;
        rlutil::msleep(sleep_ms);
    }

    // --- 遊戲結束 ---
    rlutil::setBackgroundColor(rlutil::BLACK);
    rlutil::cls();
    rlutil::locate(10, 5); cout << "Game Over!";
    rlutil::locate(10, 6); cout << "Final Score: " << score;
    rlutil::locate(10, 8); rlutil::anykey("Press any key to exit...");
    rlutil::showcursor();
    return 0;
}