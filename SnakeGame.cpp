
#include<Windows.h>
#include<iostream>
#include<thread>
#include<atomic>
#include<condition_variable>

#include<string>
#include<list>

enum COLOR
{
	FG_BLACK = 0x0000,
	FG_DARK_BLUE = 0x0001,
	FG_DARK_GREEN = 0x0002,
	FG_DARK_CYAN = 0x0003,
	FG_DARK_RED = 0x0004,
	FG_DARK_MAGENTA = 0x0005,
	FG_DARK_YELLOW = 0x0006,
	FG_GREY = 0x0007,
	FG_DARK_GREY = 0x0008,
	FG_BLUE = 0x0009,
	FG_GREEN = 0x000A,
	FG_CYAN = 0x000B,
	FG_RED = 0x000C,
	FG_MAGENTA = 0x000D,
	FG_YELLOW = 0x000E,
	FG_WHITE = 0x000F,
	BG_BLACK = 0x0000,
	BG_DARK_BLUE = 0x0010,
	BG_DARK_GREEN = 0x0020,
	BG_DARK_CYAN = 0x0030,
	BG_DARK_RED = 0x0040,
	BG_DARK_MAGENTA = 0x0050,
	BG_DARK_YELLOW = 0x0060,
	BG_GREY = 0x0070,
	BG_DARK_GREY = 0x0080,
	BG_BLUE = 0x0090,
	BG_GREEN = 0x00A0,
	BG_CYAN = 0x00B0,
	BG_RED = 0x00C0,
	BG_MAGENTA = 0x00D0,
	BG_YELLOW = 0x00E0,
	BG_WHITE = 0x00F0,
};

enum PIXEL_TYPE
{
	PIXEL_SOLID = 0x2588,
	PIXEL_THREEQUARTERS = 0x2593,
	PIXEL_HALF = 0x2592,
	PIXEL_QUARTER = 0x2591,
};

class laalSprite {

public:

	laalSprite() {

	}

	laalSprite(std::wstring sFile) {
		if (!load(sFile)) {
			createSprite(8, 8);
		}
	}

private:

	int nWidth;
	int nHeight;

	short* m_Glyphs = nullptr;
	short* m_Colors = nullptr;

public:

	void createSprite(int w, int h) {
		nWidth = w;
		nHeight = h;

		m_Glyphs = new short[w * h];
		m_Colors = new short[w * h];

		for (int i = 0; i < w * h; i++) {
			m_Glyphs[i] = L' ';
			m_Colors[i] = BG_BLACK;
		}
	}

	void clearSprite() {
		int w = spriteWidth();
		int h = spriteHeight();
		for (int i = 0; i < w * h; i++) {
			m_Glyphs[i] = L' ';
			m_Colors[i] = BG_BLACK;
		}
	}

	int spriteWidth() {
		return nWidth;
	}

	int spriteHeight() {
		return nHeight;
	}

	void setGlyph(int x, int y, short c) {
		if (x < 0 || x >= nWidth || y < 0 || y >= nHeight) {
			return;
		}

		m_Glyphs[y * nWidth + x] = c;
	}

	void setColor(int x, int y, short c) {
		if (x < 0 || x >= nWidth || y < 0 || y >= nHeight) {
			return;
		}

		m_Colors[y * nWidth + x] = c;
	}

	short getGlyph(int x, int y) {
		if (x < 0 || x >= nWidth || y < 0 || y >= nHeight) {
			return L' ';
		}
		return m_Glyphs[y * nWidth + x];
	}

	short getColor(int x, int y) {
		if (x < 0 || x >= nWidth || y < 0 || y >= nHeight) {
			return L' ';
		}
		return m_Colors[y * nWidth + x];
	}

	bool load(std::wstring sFile) {
		delete[] m_Glyphs;
		delete[] m_Colors;

		nWidth = 0;
		nHeight = 0;

		FILE* f = nullptr;
		_wfopen_s(&f, sFile.c_str(), L"rb");
		if (f == nullptr) {
			return false;
		}

		std::fread(&nWidth, sizeof(int), 1, f);
		std::fread(&nHeight, sizeof(int), 1, f);

		createSprite(nWidth, nHeight);

		std::fread(m_Colors, sizeof(short), nWidth * nHeight, f);
		std::fread(m_Glyphs, sizeof(short), nWidth * nHeight, f);

		std::fclose(f);
		return true;
	}

	bool save(std::wstring sFile) {
		FILE* f = nullptr;
		_wfopen_s(&f, sFile.c_str(), L"wb");
		if (f == nullptr)
			return false;

		fwrite(&nWidth, sizeof(int), 1, f);
		fwrite(&nHeight, sizeof(int), 1, f);
		fwrite(m_Colors, sizeof(short), nWidth * nHeight, f);
		fwrite(m_Glyphs, sizeof(short), nWidth * nHeight, f);

		fclose(f);

		return true;
	}
};

class laalGameEngine {

protected:

	int m_nScreenWidth;
	int m_nScreenHeight;

	CHAR_INFO* m_bufScreen;
	std::wstring m_sAppName;

	HANDLE m_hOriginalConsole;
	CONSOLE_SCREEN_BUFFER_INFO m_OriginalConsoleInfo;

	HANDLE m_hConsole;
	HANDLE m_hConsoleIn;

	SMALL_RECT m_rectWindow;

	bool m_bConsoleInFocus = true;

	short m_keyOldState[256] = { 0 };
	short m_keyNewState[256] = { 0 };

	struct sKeyState {
		bool bPressed;
		bool bReleased;
		bool bHeld;
	}m_keys[256];

public:

	laalGameEngine() {
		m_nScreenWidth = 80;
		m_nScreenHeight = 30;

		m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		m_hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);

		m_sAppName = L"Default";

		std::memset(m_keyOldState, 0, 256 * sizeof(short));
		std::memset(m_keyNewState, 0, 256 * sizeof(short));
		std::memset(m_keys, 0, 256 * sizeof(sKeyState));
	}

	int constructConsole(int width, int height, int fontw, int fonth) {
		if (m_hConsole == INVALID_HANDLE_VALUE) {
			return Error(L"Bad Handle");
		}

		m_nScreenWidth = width;
		m_nScreenHeight = height;

		m_rectWindow = { 0, 0, 1, 1 };
		SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow);

		COORD coord = { (short)m_nScreenHeight, (short)m_nScreenWidth };
		if (!SetConsoleScreenBufferSize(m_hConsole, coord)) {
			return Error(L"SetConsoleScreenBufferSize");
		}

		if (!SetConsoleActiveScreenBuffer(m_hConsole)) {
			return Error(L"SetConsoleActiveScreenBuffer");
		}

		//Set font size now
		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof(cfi);
		cfi.nFont = 0;
		cfi.dwFontSize.X = fontw;
		cfi.dwFontSize.Y = fonth;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;

		wcscpy_s(cfi.FaceName, L"Consolas");
		if (!SetCurrentConsoleFontEx(m_hConsole, false, &cfi)) {
			return Error(L"SetCurrentConsoleFontEx");
		}

		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (!GetConsoleScreenBufferInfo(m_hConsole, &csbi))
			return Error(L"GetConsoleScreenBufferInfo");
		if (m_nScreenHeight > csbi.dwMaximumWindowSize.Y)
			return Error(L"Screen Height / Font Height Too Big");
		if (m_nScreenWidth > csbi.dwMaximumWindowSize.X)
			return Error(L"Screen Width / Font Width Too Big");

		// Set Physical Console Window Size
		m_rectWindow = { 0, 0, (short)m_nScreenWidth - 1, (short)m_nScreenHeight - 1 };
		if (!SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow))
			return Error(L"SetConsoleWindowInfo");

		// Set flags to allow mouse input		
		if (!SetConsoleMode(m_hConsoleIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT))
			return Error(L"SetConsoleMode");

		// Allocate memory for screen buffer
		m_bufScreen = new CHAR_INFO[m_nScreenWidth * m_nScreenHeight];
		memset(m_bufScreen, 0, sizeof(CHAR_INFO) * m_nScreenWidth * m_nScreenHeight);

		SetConsoleCtrlHandler((PHANDLER_ROUTINE)closeHandler, TRUE);
		return 1;
	}

	float lerp(float firstFloat, float secondFloat, float by) {
		return firstFloat * (1 - by) + secondFloat * by;
	}

	void draw(int x, int y, short c = 0x2588, short col = 0x000F) {
		if (x >= 0 && x < m_nScreenWidth && y >= 0 && y < m_nScreenHeight) {
			m_bufScreen[y * m_nScreenWidth + x].Char.UnicodeChar = c;
			m_bufScreen[y * m_nScreenWidth + x].Attributes = col;
		}
	}

	void clip(int& x, int& y) {
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x >= m_nScreenWidth) x = m_nScreenWidth - 1;
		if (y >= m_nScreenHeight) y = m_nScreenHeight - 1;
	}

	void fill(int x1, int y1, int x2, int y2, short c = 0x2588, short col = 0x000F) {
		clip(x1, y1);
		clip(x2, y2);
		for (int x = x1; x <= x2; x++) {
			for (int y = y1; y <= y2; y++) {
				draw(x, y, c, col);
			}
		}
	}

	void drawString(int x, int y, std::wstring c, short col = 0x000F) {
		for (size_t i = 0; i < c.size(); i++) {
			m_bufScreen[y * m_nScreenWidth + x + i].Char.UnicodeChar = c[i];
			m_bufScreen[y * m_nScreenWidth + x + i].Attributes = col;
		}
	}

	void drawStringAlpha(int x, int y, std::wstring c, short col = 0x000F) {
		for (size_t i = 0; i < c.size(); i++) {
			if (c[i] != L' ') {
				m_bufScreen[y * m_nScreenWidth + x + i].Char.UnicodeChar = c[i];
				m_bufScreen[y * m_nScreenWidth + x + i].Attributes = col;
			}
		}
	}

	void drawLine(int x1, int y1, int x2, int y2, short c = 0x2588, short col = 0x000F) {
		int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
		dx = x2 - x1; dy = y2 - y1;
		dx1 = abs(dx); dy1 = abs(dy);
		px = 2 * dy1 - dx1;	py = 2 * dx1 - dy1;
		if (dy1 <= dx1) {
			if (dx >= 0) {
				x = x1; y = y1; xe = x2;
			}
			else {
				x = x2; y = y2; xe = x1;
			}

			draw(x, y, c, col);

			for (i = 0; x < xe; i++) {
				x = x + 1;
				if (px < 0)
					px = px + 2 * dy1;
				else {
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) y = y + 1; else y = y - 1;
					px = px + 2 * (dy1 - dx1);
				}
				draw(x, y, c, col);
			}
		}
		else {
			if (dy >= 0) {
				x = x1; y = y1; ye = y2;
			}
			else
			{
				x = x2; y = y2; ye = y1;
			}

			draw(x, y, c, col);

			for (i = 0; y < ye; i++) {
				y = y + 1;
				if (py <= 0)
					py = py + 2 * dx1;
				else {
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) x = x + 1; else x = x - 1;
					py = py + 2 * (dx1 - dy1);
				}
				draw(x, y, c, col);
			}
		}
	}

	void drawSprite(int x, int y, laalSprite* sprite) {

		if (sprite == nullptr) {
			return;
		}

		for (int i = 0; i < sprite->spriteWidth(); i++) {
			for (int j = 0; j < sprite->spriteHeight(); j++) {
				if (sprite->getGlyph(i, j) != L' ')
					draw(x + i, y + j, sprite->getGlyph(i, j), sprite->getColor(i, j));
			}
		}
	}

	int screenWidth() {
		return m_nScreenWidth;
	}

	int screenHeight() {
		return m_nScreenHeight;
	}

	sKeyState getKey(int nKeyId) {
		return m_keys[nKeyId];
	}

	bool isFocused() {
		return m_bConsoleInFocus;
	}

	virtual bool onUserCreate() = 0;
	virtual bool onUserUpdate(float fElapsedTime) = 0;

	virtual bool onUserDestroy() {
		return true;
	}

	void start() {

		// Start thread
		m_bAtomActive = true;

		std::thread t = std::thread(&laalGameEngine::gameThread, this);

		// Wait for the thread to be exited
		t.join();
	}

	void clearScreen() {
		fill(0, 0, m_nScreenWidth, m_nScreenHeight, L' ');
	}

	void clearScreen(int x1, int y1, int x2, int y2) {
		fill(x1, y1, x2, y2, L' ');
	}

	void setAppName(std::wstring sAppName) {
		m_sAppName = sAppName;
	}

	~laalGameEngine() {
		SetConsoleActiveScreenBuffer(m_hOriginalConsole);
		delete[] m_bufScreen;
	}

private:

	void gameThread() {

		if (!onUserCreate()) {
			m_bAtomActive = false;
		}

		auto tp1 = std::chrono::system_clock::now();
		auto tp2 = std::chrono::system_clock::now();

		while (m_bAtomActive) {

			while (m_bAtomActive) {

				// Handle timing
				tp2 = std::chrono::system_clock::now();
				std::chrono::duration<float> elapsedTime = tp2 - tp1;
				tp1 = tp2;
				float fElapsedTime = elapsedTime.count();

				// Handle keyboard input
				for (int i = 0; i < 256; i++) {
					m_keyNewState[i] = GetAsyncKeyState(i);

					m_keys[i].bPressed = false;
					m_keys[i].bReleased = false;

					if (m_keyNewState[i] != m_keyOldState[i]) {

						if (m_keyNewState[i] & 0x8000) {
							m_keys[i].bPressed = !m_keys[i].bHeld;
							m_keys[i].bHeld = true;
						}
						else {
							m_keys[i].bReleased = true;
							m_keys[i].bHeld = false;
						}
					}

					m_keyOldState[i] = m_keyNewState[i];
				}

				// Handle frame update
				if (!onUserUpdate(fElapsedTime)) {
					m_bAtomActive = false;
				}
				// Update Title & Present Screen Buffer
				wchar_t s[256];
				swprintf_s(s, 256, L"Laal Game Engine - %s - FPS: %3.2f", m_sAppName.c_str(), 1.0f / fElapsedTime);
				SetConsoleTitle(s);
				WriteConsoleOutput(m_hConsole, m_bufScreen, { (short)m_nScreenWidth, (short)m_nScreenHeight }, { 0,0 }, &m_rectWindow);
			}

			// Allow the user to free resources if they have overrided the destroy function
			if (onUserDestroy()) {
				// User has permitted destroy, so exit and clean up
				delete[] m_bufScreen;
				SetConsoleActiveScreenBuffer(m_hOriginalConsole);
				m_cvGameFinished.notify_one();
			}
			else {
				// User denied destroy for some reason, so continue running
				m_bAtomActive = true;
			}
		}
	}

protected:

	int Error(const wchar_t* msg) {
		wchar_t buf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
		SetConsoleActiveScreenBuffer(m_hOriginalConsole);
		wprintf(L"ERROR: %s\n\t%s\n", msg, buf);
		return 0;
	}

	static BOOL closeHandler(DWORD evt) {
		return true;

		// Note this gets called in a seperate OS thread, so it must
		// only exit when the game has finished cleaning up, or else
		// the process will be killed before OnUserDestroy() has finished
		if (evt == CTRL_CLOSE_EVENT) {
			m_bAtomActive = false;

			// Wait for thread to be exited
			std::unique_lock<std::mutex> ul(m_muxGame);
			m_cvGameFinished.wait(ul);
		}
	}

	// These need to be static because of the OnDestroy call the OS may make. The OS
	// spawns a special thread just for that
	static std::atomic<bool> m_bAtomActive;
	static std::condition_variable m_cvGameFinished;
	static std::mutex m_muxGame;
};

//Define static variables
std::atomic<bool> laalGameEngine::m_bAtomActive(false);
std::condition_variable laalGameEngine::m_cvGameFinished;
std::mutex laalGameEngine::m_muxGame;

/*---------------------Game Code Begins------------------------------*/

#include<vector>

class SnakeGame : public laalGameEngine {

public:

	SnakeGame() {
		setAppName(L"Snake Game");
		std::srand(std::time(0));
	}

private:

	std::list<std::pair<int, int>> snake;
	laalSprite snakeHead;
	laalSprite snakePart;
	int snakeDirection;
	laalSprite food;
	int foodPosX;
	int foodPosY;
	bool gameOver;
	int score = 0;

	void createSnakeHead() {
		snakeHead.createSprite(3, 3);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if ((i == 0 || i == 2) && j == 1) {
					snakeHead.setGlyph(i, j, 0x0000);
					snakeHead.setColor(i, j, BG_GREEN);
				}
				else if (i == 1) {
					snakeHead.setGlyph(i, j, 0x0000);
					snakeHead.setColor(i, j, j == 1 ? BG_RED : BG_GREEN);
				}
				else {
					snakeHead.setGlyph(i, j, 0x0000);
					snakeHead.setColor(i, j, BG_BLUE);
				}
			}
		}
	}

	void createSnakePart() {
		snakePart.createSprite(3, 3);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if ((i == 0 || i == 2) && j == 1) {
					snakePart.setGlyph(i, j, 0x0000);
					snakePart.setColor(i, j, BG_GREEN);
				}
				if (i == 1) {
					snakePart.setGlyph(i, j, 0x0000);
					snakePart.setColor(i, j, j == 1 ? BG_DARK_GREEN : BG_GREEN);
				}
			}
		}
	}

	void createFood() {
		food.createSprite(3, 3);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if ((i == 0 || i == 2) && j == 1) {
					food.setGlyph(i, j, 0x0000);
					food.setColor(i, j, BG_RED);
				}
				else if (i == 1) {
					food.setGlyph(i, j, 0x0000);
					food.setColor(i, j, j == 1 ? BG_YELLOW : BG_RED);
				}
				else {
					food.setGlyph(i, j, 0x0000);
					food.setColor(i, j, BG_YELLOW);
				}
			}
		}
	}

	void setFoodPosition() {
		// 12 -- 147
		// 32 -- 128

		do {
			foodPosX = 12 + 3 * (std::rand() % 46);
			foodPosY = 32 + 3 * (std::rand() % 33);
		} while (containsFood());
	}

	bool containsFood() {
		for (auto& point : snake) {
			if (point.first == foodPosX && point.second == foodPosY) {
				return true;
			}
		}
		return false;
	}
	
	void drawSnake() {
		bool headDrawn = false;
		for (auto& point : snake) {
			if (!headDrawn) {
				drawSprite(point.first - 1, point.second - 1, &snakeHead);
				headDrawn = true;
				continue;
			}
			drawSprite(point.first - 1, point.second - 1, &snakePart);
		}
	}

	void drawPlayground() {
		int p;
		for (int i = 0; i < screenHeight(); i++) {
			for (int j = 0; j < screenWidth(); j++) {
				p = std::rand() % 2;
				if (p == 0) {
					draw(j, i, L'/', FG_YELLOW);
				}
				else {
					draw(j, i, L'\\', FG_YELLOW);
				}
			}
		}
		fill(10, 30, 149, 130, L' ', BG_BLACK);
	}

	void clearPlayground() {
		clearScreen(10, 30, 149, 130);
		drawLine(10, 30, 10, 130, L'|', FG_YELLOW);
		drawLine(10, 130, 149, 130, L'-', FG_YELLOW);
		drawLine(149, 130, 149, 30, L'|', FG_YELLOW);
		drawLine(149, 30, 10, 30, L'-', FG_YELLOW);
	}

	bool eatFood() {
		return foodPosX == snake.front().first && foodPosY == snake.front().second;
	}

	void moveSnake() {
		snake.pop_back();
		//right
		if (snakeDirection == 0) {
			snake.push_front(std::make_pair(snake.front().first + 3, snake.front().second));
		}
		//down
		if (snakeDirection == 1) {
			snake.push_front(std::make_pair(snake.front().first, snake.front().second + 3));
		}
		//left
		if (snakeDirection == 2) {
			snake.push_front(std::make_pair(snake.front().first - 3, snake.front().second));
		}
		//up
		if (snakeDirection == 3) {
			snake.push_front(std::make_pair(snake.front().first, snake.front().second - 3));
		}
	}

	void growSnake() {
		//right
		if (snakeDirection == 0) {
			snake.push_front(std::make_pair(snake.front().first + 3, snake.front().second));
		}
		//down
		if (snakeDirection == 1) {
			snake.push_front(std::make_pair(snake.front().first, snake.front().second + 3));
		}
		//left
		if (snakeDirection == 2) {
			snake.push_front(std::make_pair(snake.front().first - 3, snake.front().second));
		}
		//up
		if (snakeDirection == 3) {
			snake.push_front(std::make_pair(snake.front().first, snake.front().second - 3));
		}
	}

	bool wallCollision() {
		// 10 -- 149
		// 30 -- 129
		int x = snake.front().first;
		if (snakeDirection == 0) {
			x += 1;
		}
		if (snakeDirection == 2) {
			x -= 1;
		}
		int y = snake.front().second;
		if (snakeDirection == 1) {
			y += 1;
		}
		if (snakeDirection == 3) {
			y -= 1;
		}
		if (x <= 10 || x >= 149 || y <= 30 || y >= 130) {
			return true;
		}
		return false;
	}

	bool snakeCollision() {
		int x = snake.front().first;
		int y = snake.front().second;
		int cnt = 0;
		for (auto it = snake.begin(); it != snake.end(); it++) {
			if (it->first == x && it->second == y) {
				cnt++;
			}
		}
		return cnt == 2;
	}

	void resetGame() {
		createSnakeHead();
		createSnakePart();
		createFood();
		drawPlayground();
		clearPlayground();
		snake = { {21, 80}, {18, 80}, {15, 80}, {12, 80} };
		snakeDirection = 0;
		drawSnake();
		foodPosX = 48;
		foodPosY = 80;
		drawSprite(foodPosX - 1, foodPosY - 1, &food);
		score = 0;
		updateScore();
		drawLabel();
	}

	void drawGameOver() {
		int x = 60;
		int y = 75;
		std::vector<std::wstring> arr(9);
		arr[0] = L"...........................................";
		arr[1] = L".####.####.##.##.####...####.#..#.####.####";
		arr[2] = L".#....#..#.#.#.#.#......#..#.#..#.#....#..#";
		arr[3] = L".#....#..#.#.#.#.#......#..#.#..#.#....#..#";
		arr[4] = L".#.##.####.#.#.#.####...#..#.#..#.####.###.";
		arr[5] = L".#..#.#..#.#...#.#......#..#.#..#.#....#..#";
		arr[6] = L".#..#.#..#.#...#.#......#..#.#..#.#....#..#";
		arr[7] = L".####.#..#.#...#.####...####..##..####.#..#";
		arr[8] = L"...........................................";

		for (int i = 0; i < 9; i++) {
			for (int j = 0; j < (int)arr[i].size(); j++) {
				if (arr[i][j] == L'#') {
					draw(x + j, y + i, 0x0000, BG_RED);
				}
				else {
					draw(x + j, y + i, 0x0000, BG_BLACK);
				}
			}
		}
	}

	void drawLabel() {
		int x = 10;
		int y = 20;
		std::vector<std::wstring> arr(9);
		arr[0] = L"..........................";
		arr[1] = L".####.##.#.####.#..#.####.";
		arr[2] = L".#....##.#.#..#.#..#.#....";
		arr[3] = L".#....####.#..#.#.#..#....";
		arr[4] = L".####.####.####.##...####.";
		arr[5] = L"....#.####.#..#.#.#..#....";
		arr[6] = L"....#.#.##.#..#.#..#.#....";
		arr[7] = L".####.#.##.#..#.#..#.####.";
		arr[8] = L"..........................";

		for (int i = 0; i < 9; i++) {
			for (int j = 0; j < (int)arr[i].size(); j++) {
				if (arr[i][j] == L'#') {
					draw(x + j, y + i, 0x0000, BG_RED);
				}
				else {
					draw(x + j, y + i, 0x0000, BG_BLACK);
				}
			}
		}
	}

	void drawDigit(int x, int y, int digit) {
		std::vector<std::wstring> arr(9);
		if (digit == 0) {
			arr[0] = L"......";
			arr[1] = L".####.";
			arr[2] = L".#..#.";
			arr[3] = L".#..#.";
			arr[4] = L".#..#.";
			arr[5] = L".#..#.";
			arr[6] = L".#..#.";
			arr[7] = L".####.";
			arr[8] = L"......";
		}
		if (digit == 1) {
			arr[0] = L"......";
			arr[1] = L"....#.";
			arr[2] = L"....#.";
			arr[3] = L"....#.";
			arr[4] = L"....#.";
			arr[5] = L"....#.";
			arr[6] = L"....#.";
			arr[7] = L"....#.";
			arr[8] = L"......";
		}
		if (digit == 2) {
			arr[0] = L"......";
			arr[1] = L".####.";
			arr[2] = L"....#.";
			arr[3] = L"....#.";
			arr[4] = L".####.";
			arr[5] = L".#....";
			arr[6] = L".#....";
			arr[7] = L".####.";
			arr[8] = L"......";
		}
		if (digit == 3) {
			arr[0] = L"......";
			arr[1] = L".####.";
			arr[2] = L"....#.";
			arr[3] = L"....#.";
			arr[4] = L"..###.";
			arr[5] = L"....#.";
			arr[6] = L"....#.";
			arr[7] = L".####.";
			arr[8] = L"......";
		}
		if (digit == 4) {
			arr[0] = L"......";
			arr[1] = L".#..#.";
			arr[2] = L".#..#.";
			arr[3] = L".#..#.";
			arr[4] = L".####.";
			arr[5] = L"....#.";
			arr[6] = L"....#.";
			arr[7] = L"....#.";
			arr[8] = L"......";
		}
		if (digit == 5) {
			arr[0] = L"......";
			arr[1] = L".####.";
			arr[2] = L".#....";
			arr[3] = L".#....";
			arr[4] = L".####.";
			arr[5] = L"....#.";
			arr[6] = L"....#.";
			arr[7] = L".####.";
			arr[8] = L"......";
		}
		if (digit == 6) {
			arr[0] = L"......";
			arr[1] = L".#....";
			arr[2] = L".#....";
			arr[3] = L".#....";
			arr[4] = L".####.";
			arr[5] = L".#..#.";
			arr[6] = L".#..#.";
			arr[7] = L".####.";
			arr[8] = L"......";
		}
		if (digit == 7) {
			arr[0] = L"......";
			arr[1] = L".####.";
			arr[2] = L"....#.";
			arr[3] = L"....#.";
			arr[4] = L"....#.";
			arr[5] = L"....#.";
			arr[6] = L"....#.";
			arr[7] = L"....#.";
			arr[8] = L"......";
		}
		if (digit == 8) {
			arr[0] = L"......";
			arr[1] = L".####.";
			arr[2] = L".#..#.";
			arr[3] = L".#..#.";
			arr[4] = L".####.";
			arr[5] = L".#..#.";
			arr[6] = L".#..#.";
			arr[7] = L".####.";
			arr[8] = L"......";
		}
		if (digit == 9) {
			arr[0] = L"......";
			arr[1] = L".####.";
			arr[2] = L".#..#.";
			arr[3] = L".#..#.";
			arr[4] = L".####.";
			arr[5] = L"....#.";
			arr[6] = L"....#.";
			arr[7] = L"....#.";
			arr[8] = L"......";
		}
		for (int i = 0; i < 9; i++) {
			for (int j = 0; j < (int)arr[i].size(); j++) {
				if (arr[i][j] == L'#') {
					draw(x + j, y + i, 0x0000, BG_RED);
				}
				else {
					draw(x + j, y + i, 0x0000, BG_BLACK);
				}
			}
		}
	}

	void updateScore() {
		int x = 133;
		int y = 20;

		int sc = score;
		int c = sc % 10;
		sc /= 10;
		int b = sc % 10;
		sc /= 10;
		int a = sc % 10;
		
		drawDigit(x, y, a);
		drawDigit(x + 5, y, b);
		drawDigit(x + 10, y, c);
	}

protected:
	
	virtual bool onUserCreate() {
		gameOver = false;
		resetGame();
		return true;
	}

	virtual bool onUserUpdate(float fElapsedTime) {
		if (gameOver) {
			if (getKey(VK_SPACE).bPressed) {
				resetGame();
				gameOver = false;
			}
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(75));
		if (getKey(VK_RIGHT).bPressed && snakeDirection % 2 == 1) {
			snakeDirection = 0;
		}
		if (getKey(VK_DOWN).bPressed && snakeDirection % 2 == 0) {
			snakeDirection = 1;
		}
		if (getKey(VK_LEFT).bPressed && snakeDirection % 2 == 1) {
			snakeDirection = 2;
		}
		if (getKey(VK_UP).bPressed && snakeDirection % 2 == 0) {
			snakeDirection = 3;
		}
		
		if (wallCollision() || snakeCollision()) {
			gameOver = true;
			drawGameOver();
		}
		else {
			if (eatFood()) {
				setFoodPosition();
				growSnake();
				score++;
				updateScore();
			}
			moveSnake();
			clearPlayground();
			drawSnake();
			drawSprite(foodPosX - 1, foodPosY - 1, &food);
		}
		return true;
	}
};

int main() {
	SnakeGame game;
	game.constructConsole(160, 160, 4, 4);
	game.start();
	return 0;
}
