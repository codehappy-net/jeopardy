/****

	jeopardy.h
	
	v2 of Chris Street's "Jeopardy!" simulation, this one written for libcodehappy
	AND with LLM-based artificially intelligent opponents!

	Source code copyright (c) 2011-2023 Chris Street

****/
#ifndef JEOPARDY_H
#define JEOPARDY_H

#define CODEHAPPY_SDL
#ifndef CODEHAPPY_WASM
#define CODEHAPPY_NATIVE
#define CODEHAPPY_CUDA
#endif
#include <libcodehappy.h>

const int BOARD_CLR = RGB_NO_CHECK(0x00, 0x00, 0x73);
const int WHITE_CLR = RGB_NO_CHECK(0xff, 0xff, 0xff);
const int RED_CLR = RGB_NO_CHECK(0xff, 0x00, 0x00);
const int BLACK_CLR = RGB_NO_CHECK(0x00, 0x00, 0x00);

const int XSPACING = 1;
const int BOARDTHICK = 4;
const int CATPADDING = 5;
const int TIMERLENGTH = 300;
const int TIMERHEIGHT = 20;

const int GAME_JEOPARDY = 0;
const int GAME_QUIZ = 1;
const int GAME_CHALLENGE = 2;
const int GAME_JEOPARDY3 = 3;
const int GAME_BUZZER = 4;

using std::ofstream;
using std::ifstream;
using std::ostream;
using std::istream;
using std::vector;
using std::endl;

typedef enum _mode
{
	mode_main_menu,
	mode_category_reveal,
	mode_board_reveal,
	mode_board,
	mode_animating_clue,
	mode_clue,
	mode_wager,
	mode_instructions,
	mode_fj3rd,
	mode_fj2nd,
	mode_fj1st,
	mode_opponent_dd_wager,
	mode_opponent_dd_answer,
	mode_opponent_dd_question,
	mode_game_end,
	mode_introduce_contestants,
	mode_buzzer_clue,
	mode_buzzer_params,
	mode_buzzer_show_results,
	mode_specialtyselect,
	mode_aiplayer_answers,
	mode_aiopponent_dd_answer,
} mode;

typedef enum _sfx
{
	sfx_board = 0,
	sfx_double1,
	sfx_double2,
	sfx_double3,
	sfx_double4,
	sfx_double5,
	sfx_double6,
	sfx_double7,
	sfx_double8,
	sfx_double9,
	sfx_double10,
	sfx_double11,
	sfx_double12,
	sfx_double13,
	sfx_double14,
	sfx_double15,
	sfx_double16,
	sfx_reveal,
	sfx_round,
	sfx_time,
	sfx_think,
	sfx_think2,
	sfx_think3,
	sfx_round1intro,
	sfx_round1introalt1,
	sfx_round1introalt2,
	sfx_round1introalt3,
	sfx_round2intro,
	sfx_round2introalt1,
	sfx_round2introalt2,
	sfx_round2introalt3,
	sfx_round3intro,
	sfx_boardstart1,
	sfx_boardstart2,
	sfx_boardstart3,
	sfx_ddcorrect1,
	sfx_ddcorrect2,
	sfx_ddcorrect3,
	sfx_ddcorrect4,
	sfx_ddcorrect5,
	sfx_ddcorrect6,
	sfx_ddcorrect7,
	sfx_wager1,
	sfx_wager2,
	sfx_wager3,
	sfx_wager4,
	sfx_wager5,
	sfx_wager6,
	sfx_wager7,
	sfx_wager8,
	sfx_wager9,
	sfx_wager10,
	sfx_selection,
	sfx_gameintro1,
	sfx_gameintro2,
	sfx_gameintro3,
	sfx_canruncategory,
	sfx_playerchris,
	sfx_playerjohn,
	sfx_playerbrad,
	sfx_playerjonathan,
	sfx_playerkay,
	sfx_playermary,
	sfx_playerjoshua,
	sfx_playeralex,
	sfx_playerwillie,
	sfx_playeromar,
	sfx_playerlisa,
	sfx_playervan,
	sfx_playerwilliam,
	sfx_playerchristian,
	sfx_playerkate,
	sfx_playerbrandon,
	sfx_playerphil,
	sfx_playerhillary,
	sfx_playercarl,
	sfx_playercecilia,
	sfx_playeralan,
	sfx_playeranthony,
	sfx_playerjenny,
	sfx_playeradam,
	sfx_playerben,
	sfx_playerdavid,
	sfx_playererin,
	sfx_playerlan,
	sfx_playermark,
	sfx_playermelinda,
	sfx_playermichael,
	sfx_playerscott,
	sfx_playerkaren,
	sfx_playersusan,
	sfx_playermitch,
	sfx_playerdamien,
	sfx_playerrachel,
	sfx_playerjudy,
	sfx_playerian,
	sfx_playerpaul,
	sfx_playersarah,
	sfx_playerjanet,
	sfx_playerkathleen,
	sfx_playeraric,
	sfx_playervirginia,

	sfx_max
} sfx;

extern const char *playernames[];

const int NUM_ADDL_PLAYERS = 44;

const int count_right_fx = 80;
const int count_wrong_fx = 15;

class JeopardyFont {
private:
	Font *fontdata;
	vector<int> redx;
	vector<int> charx;
	vector<int> chars;
	bool valid;
	bool catfont;

public:
	JeopardyFont(const char* pathname);
	~JeopardyFont();
	SBitmap *RenderWord(const char *word, bool type_buffer = false);
	SBitmap *RenderCategory(char *cat);
};

struct Response {
	int type;
	int date;
	int time;
	int catid;
	int value;
	int round;
	int ringin;
	char response[256];
	int right;
	bool added;
};

struct GameStats {
	int type;
	int date;
	int time;
	int qright;
	int qwrong;
	int ddright;
	int ddwrong;
	int valright;
	int valwrong;
	int coryat;
	int fjright;
	int score;
	int scoreopp1;
	int scoreopp2;
	int gameseries;
	int coryatdd;
	int rqright[2][5];
	int rqwrong[2][5];
	int rddright[2][5];
	int rddwrong[2][5];
};

const int BUZZERSIMCLUES = 51;

struct buzzscore {
	int nattempt;
	int ntotal;
	int ncall;
	int nearly;
	int n5;
	int n10;
	int n15;
	int tdelay;
};

/*** Object that handles sound effects. ***/
class SoundFX {
public:
	SoundFX();
	void play_sfx(WavFile& wf);
	void update();

private:
	WavFile* sfx[8];
	int ticks_due[8];
	int ticks_start[8];
};

/*** Data for the Llama generation thread. ***/
struct GenData {
	GenData();
	Llama* llama;
	char response[512];
	bool generating;
	bool generation_requested;
	std::mutex mtx;
	std::string category;
	std::string clue;
};

/*** The Jeopardy! game. ***/
class Jeopardy {
public:
	Jeopardy();
	~Jeopardy();

	bool fGfxInit;
	int w_x;
	int w_y;

	int m_x;
	int m_y;

	int s_c;	// selection: category (0-5)
	int s_a;	// selection: amount (0-4)
	int s_cl;	// last selection
	int s_al;	// last selection

	int max_catid;
	int *catstatus;
	SBitmap *dollaramounts[10];	// 200, 400, 600, 800, 1000, 400, 800, 1200, 1600, 2000
	bool catdrawn;

	mode current_game_mode;

	bool game_hard;
	bool game_specialty;
	char specialty[128];
	vector<int> specialtycats;

	int cats[12];

	JeopardyFont* category_font;
	JeopardyFont* answer_font;

	SBitmap* screen;

	sqlite3 *db;

	int tick_start;
	SBitmap *clue_bmp;
	SBitmap *ques_bmp;
	SBitmap *dd_bmp;
	int cx1, cx2, cy1, cy2;	// starting rectangle for clue
	bool selected[6][5];
	bool revealed[6][5];
	bool revealsound;
	bool rdown;

	std::string category_names[6];
	std::string cur_clue_text;

	// the language model to use for answers
	Llama* llama;
	double temp;

	// sound effects
	WavFile sound_fx[sfx_max];
	WavFile right_fx[count_right_fx];
	WavFile wrong_fx[count_wrong_fx];

	// text-to-speech stuff

	// round of play
	int round;

	// the Daily Double(s)
	SBitmap *dd;
	int dd_c[2], dd_a[2];
	bool ddsel;

	int catreveal;

	// Final Jeopardy! stuff
	char *final_cat;
	char *final_clue;
	char *final_question;
	bool thinkmusic;
	bool fjdone;

	// game play stuff
	bool isgame;		// vs. practice/"host" mode
	bool fasttimer;		// vs. more generous timer
	int playerscore;
	int oppscore[2];
	char typebuffer[1024];
	bool clueshown;
	bool inputstart;
	bool timesound;
	bool showanswer;
	bool showamount;
	bool answerchecked;
	char accept[1024];
	int wager;			// wager for DD and Final Jeopardy
	bool drawtimer;
	int timerlen;
	int tcount;
	bool introsound;
	bool boardsound;
	bool boardsound2;
	int game_type;		// 0 = Jeopardy! game, 1 = contestant quiz
	int pcoryat;		// the player's Coryat score, in a solo game
	int dcoryat;
	int buzzerscore;
	Response response;
	GameStats gamestats;

	// 3-player mode: who can ring in?
	bool canring[3];
	bool thinksknows[2];
	bool actuallyknows[2];
	int playerid[3];
	int FinalJeopardyWager(int ip);
	int fjwager[2];
	bool fjright[2];
	bool fjgen[2];
	int plc[3];
	int fjs[3];
	bool lastqright;
	int control;
	int tkp[2][5];
	int akp[2][5];
	int lastcatsel;
	int ddwagers;
	int gamewon;
	int winnings;
	int place;
	int judgedock;
	int ai_judge;
	int ai_judgeamt;
	bool goup;
	int coccup;
	int cfrom;
	int ioccup[2];
	int ifrom[2];
	int acoryat[2];
	ofstream buzzersimlog;
	int namecall;
	int voicecall;
	int tickcall;
	int calldonetick;
	int op_ringin;
	bool correctok;

	// stuff for playing proper DD sounds
	bool ddseen;
	bool wasleading;
	bool willlead;
	bool istruedd;

	// main menu
#ifdef CODEHAPPY_WASM	
	static const int MM_OPTIONS = 2;
#else
	static const int MM_OPTIONS = 8;
#endif	

	int opy[MM_OPTIONS];	// option y-positions
	int minx;	// minimum x-position
	int mmsel;

	// contestant quiz stuff
	int nquestions;
	int nright;
	char *lastq;

	// buzzer clue mode stuff
	bool buzzload;
	int nclues;
	char **buzzclues;
	int mdelay;
	int mdelayb;	// base buzzer delay
	vector<int> iclue;
	int ndone;
	WavFile *reading;
	int readinglen;
	int hsec;
	int hsec_first;
	bool showtime;
	bool lightson;
	int lighttime;
	int voicen;
	int voiced;
	int voicet;
	int voicef;
	int driftv;
	int delayv;
	buzzscore buzzsc;
	
	void DrawBoard(void);
	void ClueBitmap(bool question);
	void CreateClueBitmap(char *str, bool question);
	void AnimateClue(void);
	void RoundBegin(void);
	void DDBitmap(void);
	void DoReveal(void);
	void DoCategoryReveal(void);
	bool CheckRound(void);
	void FJHandler(void);
	void DrawTypeBuffer(void);
	void DrawAITypeBuffer(void);
	void DrawScore(void);
	void PrepareToAcceptInput(void);
	void TimerOut(void);
	void CheckAnswer(void);
	void CheckAIAnswer(bool daily_double);
	void DrawWager(void);
	bool ValidateWager(void);
	void LoadSoundFx(void);
	void PlayAlexResponse(bool right);
	void DrawMainMenu(void);
	void DrawInstructions(void);
	void DrawBuzzerDelayParam(void);
	void DrawBuzzerResults(void);
	void FetchNextClue(bool ts);
	void DrawExamStatus(void);
	void InitClue3Player(void);
	int WhoRings(bool player);
	void OpponentRingin(int ip);
	void DrawFJReveal(int pl);
	void BoardControl(void);
	void SelectClue(void);
	void DrawDDWager(void); 
	int OpponentDDWager(void);
	void DrawGameEnd(void);
	void CheckWinnings(void);
	int CalculateContestantAverageCoryat(int iopp);
	int CalculateAverageCombinedCoryat(int iopp);
	void PlayIntroSwoosh(void);
	void DrawContestants(void);
	void CreateContestants(void);
	void BuzzerClue(void);
	void InitGameStats(void);
	void FillDateTime(int &date, int &time);
	void SaveGame(void);
	void SaveCatmap(void);
	void LoadCatmap(void);
	void DrawSpecialty(void);
	void FindSpecialties(void);
	void SelectCategories(void);
	void TurnBuzzersOff(void);
	bool AllBuzzersOff(void);
	void ForceRedraw(bool all = false);
	void DrawCenteredText(const char* str, int y);
	void Initialize(const char* model_path, double tmp, bool twogpu, int screen_w, int screen_h);
	void Draw(void);
	void TimerCallback(void);
	void Size(int cx, int cy);
	void MouseMove(int x, int y);
	void LButtonUp(void);
	void RButtonDown(void);
	void RButtonUp(void);
	void KeyDown(int keycode);
	void KeyUp(int keycode);
};


#endif  // JEOPARDY_H
/* end jeopardy.h */
