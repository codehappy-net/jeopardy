/****

	jeopardy.cpp
	
	v2 of Chris Street's "Jeopardy!" simulation, this one written for libcodehappy
	AND with LLM-based artificially intelligent opponents!

	Source code copyright (c) 2011-2023 Chris Street

****/
#include "jeopardy.h"

#define APP_WIDTH	1200
#define APP_HEIGHT	900


/* contestant names and other flavor text */
const char *playernames[] = {
	"CHRIS", "JOHN", "BRAD", "JONATHAN", "KAY", "MARY", "JOSHUA", "ALEX", "WILLIE", "OMAR", "LISA", "VAN", "WILLIAM", "CHRISTIAN",
	"KATE", "BRANDON", "PHIL", "HILLARY", "CARL", "CECILIA", "ALAN", "ANTHONY", "JENNY", "ADAM", "BEN", "DAVID", "ERIN", "LAN",
	"MARK", "MELINDA", "MICHAEL", "SCOTT", "KAREN", "SUSAN", "MITCH", "DAMIEN", "RACHEL", "JUDY", "IAN", "PAUL", "SARAH", "JANET",
	"KATHLEEN", "ARIC", "VIRGINIA",
	};

static const char *occupations[] = {
	"AN ACCOUNTANT", "AN ACTOR", "AN AGRIBUSINESS DIRECTOR", "AN ARCHITECT", "AN ATTORNEY", "A BALLROOM DANCE INSTRUCTOR", "A BOXING INSTRUCTOR",
	"A CHEF", "A COLLEGE DEAN", "A COLLEGE PROFESSOR",  "A COMMUNITY ORGANIZER", "A CONGRESSIONAL PAGE", "A CONSERVATION JOURNALIST",
	"A DIRECTOR OF ADMISSIONS", "A DUELING PIANIST", "AN EDITOR", "AN EDUCATIONAL CONSULTANT", "AN ENGLISH TEACHER",  "A FEDERAL INVESTIGATOR",
	"A FEDERAL LAW CLERK", "A FINANCIAL ANALYST", "A FOREIGN SERVICE OFFICER", "A FREELANCE WRITER", "A GENERAL LITIGATION ATTORNEY",
	"A GOVERNMENT ANALYST", "A GRADUATE STUDENT", "A HEALTHCARE WORKER", "A HIGH SCHOOL ENGLISH TEACHER", "A HIGH SCHOOL LIBRARIAN",
	"A HIGH SCHOOL THEOLOGY TEACHER", "A HISTORY TEACHER", "A HOMICIDE DETECTIVE", "AN ICE CREAM TRUCK DRIVER", "AN INFORMATION TECHNOLOGY MANAGER",
	"A STUDENT COORDINATOR", "AN INTERNET MARKETER", "AN INTERNET SECURITY ENGINEER", "A JOURNALIST", "A KINDERGARTEN TEACHER",
	"A LAW SCHOOL GRADUATE", "A LETTER CARRIER", "A MANAGEMENT CONSULTANT", "A MARKETING DIRECTOR", "A MARKETING MANAGER",
	"A MEDICAL MALPRACTICE ATTORNEY", "A MEDIEVALIST", "A MENTAL HEALTH THERAPIST", "A MYSTERY AND SUSPENSE WRITER", "A NANNY",
	"A NAVY ANALYST", "A PROJECT COORDINATOR", "A NURSING STUDENT", "AN OBSTETRICIAN", "AN ONCOLOGIST", "AN ORCHESTRA CONDUCTOR",
	"A PARALEGAL", "A PERSONAL ASSISTANT", "A PERSONAL TRAINER", "A PETSITTER", "A PHARMACEUTICAL SCIENTIST", "A PLASTIC SURGEON",
	"A POLICY ANALYST", "A PROJECT MANAGER", "A PSYCHIATRIST", "A PUBLICIST", "A RESEARCH CHEMIST", "A RETAIL SALES ASSOCIATE",
	"A SECRETARY", "A SINGER", "A SOFTWARE ARCHITECT", "A SOFTWARE DEVELOPER", "A SOFTWARE TESTER", "A STAND-UP COMEDIAN", "A STAY-AT-HOME PARENT", 
	"A STUDENT", "A TECHNOLOGY TRAINER", "A TEXTBOOK EDITOR", "A TRUCK DRIVER", "A UNIVERSITY LIBRARIAN", "A VIDEO PRODUCER", "A VIOLINIST",
	"A VOLUNTEER", "A WRITER","A ZOO DOCENT",
	nullptr
};

static const char *wherefrom[] = {
	"ALBUQUERQUE, NEW MEXICO", "ANNANDALE, VIRGINIA", "ANNAPOLIS, MARYLAND", "ARDSLEY, NEW YORK", "ATLANTIC HIGHLANDS, NEW JERSEY",
	"AUSTIN, TEXAS", "BALA CYNWYD, PENNSYLVANIA", "BALTIMORE, MARYLAND",  "BETHESDA, MARYLAND", "BOISE, IDAHO", "BROAD BROOK, CONNECTICUT",
	"BUFFALO, NEW YORK", "BURLINGTON, VERMONT", "CHESTERFIELD, VIRGINIA", "CHEVY CHASE, MARYLAND", "CHICAGO, ILLINOIS", "CHICKASAW, ALABAMA",
	"CINCINNATI, OHIO", "CLINTON, NEW YORK", "COLUMBIA, MISSOURI", "COLUMBUS, OHIO", "CORONADO, CALIFORNIA", "CULVER CITY, CALIFORNIA",
	"CUMMAQUID, MASSACHUSETTS", "DALLAS, TEXAS", "DAYTON, OHIO", "DETROIT, MICHIGAN", "EAST MEADOW, NEW YORK", "ELLICOTT CITY, MARYLAND",
	"ELMHURST, NEW YORK", "EXETER, NEW HAMPSHIRE", "FAIRBANKS, ALASKA", "FORT WAYNE, INDIANA", "GLASTONBURY, CONNECTICUT", 
	"GREENVILLE, SOUTH CAROLINA", "HOUSTON, TEXAS", "KENOSHA, WISCONSIN", "LAFAYETTE, INDIANA", "LAKEWOOD, COLORADO", "LITTLE ROCK, ARKANSAS",
	"LONDON, ENGLAND", "LOS ALAMOS, NEW MEXICO", "MADISON, WISCONSIN", "MARIETTA, GEORGIA", "MIAMI BEACH, FLORIDA", "MILAN, MICHIGAN", "MOSCOW, IDAHO",
	"NEW YORK, NEW YORK", "NORTH HOLLYWOOD, CALIFORNIA", "OAK RIDGE, TENNESSEE", "PALO ALTO, CALIFORNIA", "PLACENTIA, CALIFORNIA",
	"PORTLAND, OREGON", "RALEIGH, NORTH CAROLINA",  "SACRAMENTO, CALIFORNIA", "SAN DIEGO, CALIFORNIA", "SANDY SPRINGS, GEORGIA",
	"SAN FRANCISCO, CALIFORNIA", "SANTA BARBARA, CALIFORNIA", "SILVER SPRING, MARYLAND", "SEATTLE, WASHINGTON", "SILVA, NORTH CAROLINA",
	"SPOKANE, WASHINGTON", "STILLWATER, OKLAHOMA", "ST. LOUIS, MISSOURI",  "STOCKTON, CALIFORNIA", "TACOMA, WASHINGTON", "TORONTO, ONTARIO",
	"TROY, NEW YORK", "VANCOUVER, BRITISH COLUMBIA", "VIENNA, VIRGINIA", "WASHINGTON, D. C.", "WEST DEPFORD, NEW JERSEY", "WHEATON, ILLINOIS",
	"WINNEBAGO, ILLINOIS",
	nullptr
};


// on each screen refresh, this value is updated to indicate the number of hundredths of seconds elapsed. Used for animations and timers.
volatile int ticks = 0;

#define RINGIN_DEPENDENT
static bool _adjsc = false;
static Jeopardy* clback = nullptr;
static bool _timerlock = false;
const int dep100 = 80;			// dependent coefficient (in hundredths)
static SoundFX soundfx;
static GenData gendata;

/*** Response generation thread implementation ***/
GenData::GenData() {
	llama = nullptr;
	response[0] = 0;
	generating = false;
	generation_requested = false;
}

#ifdef CODEHAPPY_CUDA
std::string jeopardy_isn(const std::string &category, const std::string& clue) {
	std::string isn;

	isn = "Congratulations! You are a contestant in a game of 'Jeopardy!', America's Favorite Quiz Show!(tm) Remember the rules: always "
	      "give your response in the form of a question. To win the game, simply answer the clue to the best of your ability!\n\n";

	if (!category.empty()) {
		// category provided, let's add extra context
		isn += "The category for this clue is: ";
		isn += category;
		isn += "\n\n";
		// Sometimes we can give additional hints about the rules based on the category name!
		if (strchr(category.c_str(), '\"') != nullptr) {
			isn += "Remember that the quotation marks in the category name mean that the correct response includes the part in quotes!\n\n";
		}
	}

	isn += "Here is the clue:\n\n";
	isn += clue;
	isn += "\n";

	// for debugging purposes, let's print the prompt conditioning to the console for now.
//	std::cout << isn;

	return isn;
}

const int JEOPARDY_MAX_TOKENS = 128;

void generation_callback(const char* next_token) {
	gendata.mtx.lock();

	/* concatenate this token onto our response buffer */
	char* w;
	const char* we, * x;
	w = gendata.response + strlen(gendata.response);
	we = gendata.response + 511;
	x = next_token;
	while (w < we) {
		bool ok = true;
		if (*x == '\n')
			ok = false;
		if (ok)
			*w = toupper(*x);
		if (!(*x))
			break;
		if (ok)
			++w;
		++x;
	}
	gendata.response[511] = '\000';

	gendata.mtx.unlock();
}

void generation_thread() {
	bool generate;
	std::string cat, clue;
	Llama* llama;
	forever {
		generate = false;
		gendata.mtx.lock();
		if (gendata.generation_requested) {
			gendata.generation_requested = false;
			gendata.response[0] = '\000';
			llama = gendata.llama;
			cat = gendata.category;
			clue = gendata.clue;
			generate = true;
		}
		gendata.mtx.unlock();

		if (generate) {
			std::string isn = jeopardy_isn(cat, clue);
			llama->isn_prompt(isn);
			llama->generate_tokens(JEOPARDY_MAX_TOKENS, false, generation_callback);
			gendata.mtx.lock();
			gendata.generating = false;
			gendata.mtx.unlock();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void request_generation(Llama* llama, const std::string& category, const std::string& clue) {
	gendata.mtx.lock();
	assert(!gendata.generating);
	gendata.llama = llama;
	gendata.generation_requested = true;
	gendata.generating = true;
	gendata.category = category;
	gendata.clue = clue;
	gendata.mtx.unlock();
}

std::string current_generation_str() {
	std::string ret;
	gendata.mtx.lock();
	ret = gendata.response;
	gendata.mtx.unlock();
	return ret;
}

bool currently_generating() {
	bool ret = false;
	gendata.mtx.lock();
	ret = gendata.generating;
	gendata.mtx.unlock();
	return ret;
}

#else

// !CODEHAPPY_CUDA: stubs
void request_generation(Llama* llama, const std::string& category, const std::string& clue) {
}

std::string current_generation_str() {
	std::string ret;
	return ret;
}

bool currently_generating() {
	return false;
}
#endif  // !CODEHAPPY_CUDA

/*** SoundFX class implementation ***/
SoundFX::SoundFX() {
	for (int e = 0; e < 8; ++e) {
		sfx[e] = nullptr;
		ticks_due[e] = 0;
		ticks_start[e] = 0;
	}
}

void SoundFX::update() {
	int t = ticks;
	for (int e = 0; e < 8; ++e) {
		if (sfx[e] == nullptr) {
			continue;
		}
		if (ticks_start[e] > 0 && t >= ticks_start[e]) {
			ticks_start[e] = 0;
			sfx[e]->play_wav();
		}
		if (t >= ticks_due[e]) {
			ticks_due[e] = 0;
			ticks_start[e] = 0;
			sfx[e] = nullptr;
		}
	}
}

void SoundFX::play_sfx(WavFile& wf) {
	int len_100ths = (wf.len * 100) / (wf.sample_rate() * wf.num_channels() * 2) + 5;
	int idx = -1;
	int start_tick = ticks;

	for (int e = 0; e < 8; ++e) {
		if (sfx[e] == nullptr && idx < 0)
			idx = e;
		if (sfx[e] != nullptr && ticks_due[e] > start_tick)
			start_tick = ticks_due[e];
	}
	if (idx < 0)
		return;
	ticks_start[idx] = start_tick;
	ticks_due[idx] = start_tick + len_100ths;
	sfx[idx] = &wf;
}

/*** JeopardyFont class implementation ***/

JeopardyFont::JeopardyFont(const char* pathname) {
	this->fontdata = new Font(pathname);
	this->valid = true;
	this->catfont = (!strcmp(pathname, "swiss911.ttf"));
}

JeopardyFont::~JeopardyFont() {
	if (!this->valid)
		return;
	valid = false;
	delete fontdata;
}

SBitmap *JeopardyFont::RenderCategory(char *cat) {
	vector<SBitmap *> bmaps;
	SBitmap *bm;
	SBitmap *rdr;
	char tl[256];
	int ch;
	int ci;
	int ln;
	int x;
	int y;
	int e;
	char *ls;
	int cl;

	ch = 0;
	ci = 0;
	bmaps.clear();
	while (cat[ch]) {
		if (cat[ch] == '\n' && ci > 0) {
			tl[ci] = 0;
			bm = this->RenderWord(tl);
			ci = 0;
			bmaps.push_back(bm);
			++ch;
			continue;
		} else if (isspace((unsigned char)cat[ch])) {
			cl = ci + 1;
			while (cat[cl] && !isspace(cat[cl]))
				cl++;
			if ((this->catfont && (ci > 12 || cl > 14)) ||
				(!this->catfont && (ci > 18 || cl > 22))) {
				// render a new line
				tl[ci] = 0;
				bm = this->RenderWord(tl);
				ci = 0;
				bmaps.push_back(bm);
				++ch;
				continue;
			}
		}
		tl[ci] = cat[ch];
		++ch;
		++ci;
	}
	if (ci > 0) {
		tl[ci] = 0;
		bm = this->RenderWord(tl);
		ci = 0;
		bmaps.push_back(bm);
	}

	x = 0;
	y = 0;
	for (e = 0; e < bmaps.size(); ++e) {
		if (bmaps[e]->width() > x)
			x = bmaps[e]->width();
		y += bmaps[e]->height();
		if (e + 1 < bmaps.size())
			y += 4;
	}

	rdr = new SBitmap(x, y);
	rdr->clear(BOARD_CLR);
	y = 0;
	for (e = 0; e < bmaps.size(); ++e) {
		bmaps[e]->blit(rdr, 0, 0, (x - bmaps[e]->width()) / 2, y, bmaps[e]->width(), bmaps[e]->height());
		y += bmaps[e]->height();
		y += 4;
		delete bmaps[e];
	}

	return(rdr);
}

static SBitmap* typebuf_bmp = nullptr;

SBitmap *JeopardyFont::RenderWord(const char *word, bool type_buffer) {
	const int height = (catfont ? 64 : 36);
	char wordu[1024];

	strncpy(wordu, word, 1023);
	__strupr(wordu);

	SBitmap* o = nullptr, * ret = nullptr;
	if (type_buffer) {
		if (is_null(typebuf_bmp)) {
			typebuf_bmp = new SBitmap(APP_WIDTH, 80);
		}
		ret = typebuf_bmp;
	} else {
		o = fontdata->render_cstr(wordu, fontdata->font_size_for_height(height), false, nullptr);
		ret = new SBitmap(o->width(), o->height());
	}

	ret->clear(BOARD_CLR);
	ret->render_text(wordu, fontdata, C_WHITE, height, CENTERED_BOTH);
	if (!is_null(o)) {
		delete o;
	}
	return ret;
}

void AdjBmp(SBitmap *bmp) {
	/* Attempt to clean up the bitmap a little by normalizing the colors. */
	int x;
	int y;
	RGBColor c;
	int r, g, b;

	for (y = 0; y < bmp->height(); ++y) {
		for (x = 0; x < bmp->width(); ++x) {
			c = bmp->get_pixel(x, y);

			if (c == BOARD_CLR || c == WHITE_CLR || c == RED_CLR)
				continue;		// that's fine

			r = RGB_RED(c);
			g = RGB_GREEN(c);
			b = RGB_BLUE(c);

			if (b > 60 && (r + g) < 50) {
				// What is dark blue?
				bmp->put_pixel(x, y, BOARD_CLR);
			}
			else if (b > 140 && (r + g) < 250) {
				// What is (also) dark blue?
				bmp->put_pixel(x, y, BOARD_CLR);
			}
		}
	}
}

/*** SQLite callbacks ***/

static int *catarray = nullptr;
static int cat_callback1(void *v, int ncols, char **cols, char **colname) {
	/*
		determine which round each category belongs to
	*/
	catarray[atoi(cols[0])] = atoi(cols[1]);
	return(0);
}

static int *cluecount = nullptr;
static int cat_callback2(void *v, int ncols, char **cols, char **colname) {
	/*
		Eliminate some categories that include links (video/audio).
	*/
	cluecount[atoi(cols[0])]++;	// so we know which categories do not have all clues revealed
	return(0);
}

static int *cat_responses = nullptr;
static int cat_callback3(void *v, int ncols, char **cols, char **colname) {
	/*
		For Jeopardy! hard mode: select categories where I haven't had many correct responses.
	*/
	int catid;
	int ringin;
	int right;

	catid = atoi(cols[3]);
	ringin = atoi(cols[6]);
	right = atoi(cols[8]);

	if (ringin != 0) {
		if (right > 0)
			cat_responses[catid]++;
		else
			cat_responses[catid]--;
	}

	return(0);
}

void RemoveStr(char *str, const char *r) {
	char *w;

	w = strstr(str, r);
	while (w) {
		strcpy(w, w + strlen(r));
		w = strstr(str, r);
	}
}

void RemoveNewlines(char *str) {
	char *w;

	w = strstr(str, "<br />");

	while (w) {
		strcpy(w + 1, w + 6);
		*w = '\n';
		w = strstr(str, "<br />");
	}
}

void NormalizeStr(char *str) {
	char *w;
	char *w2;

	RemoveStr(str, "amp;");
	RemoveStr(str, "<u>");
	RemoveStr(str, "<i>");
	RemoveStr(str, "</u>");
	RemoveStr(str, "</i>");
	RemoveNewlines(str);
	RemoveStr(str, "<b>");
	RemoveStr(str, "</b>");

	w = strchr(str, '<');
	while (w) {
		w2 = strchr(w + 1, '>');
		if (w2)
			strcpy(w, w2 + 1);
		else
			break;
		w = strchr(str, '<');
	}

	// remove parenthetical notes about the Clue Crew
	w = strstr(str, "of the Clue Crew");
	if (w != NULL) {
		w2 = strchr(w, ')');
		while (w > str && *w != '(')
			--w;
		if (w2) {
			w2++;
			strcpy(w, w2);
		}
	}
}

char strret[4096] = {0};
static int str_callback(void *v, int ncols, char **cols, char **colname) {
	char *w;

	strcpy(strret, cols[0]);

	NormalizeStr(strret);
	
	return(0);
}

int _lookfor;
static int clue_callback(void *v, int ncols, char **cols, char **colname) {
	if (_lookfor == 0) {
		strcpy(strret, cols[0]);
		NormalizeStr(strret);
		return(1);
	}
	--_lookfor;
	return(0);
}

int intRet_db;
static int int_callback(void *v, int ncols, char **cols, char **colname) {
	if (cols[0] == NULL)
		return(0);
	intRet_db = atoi(cols[0]);
	return(0);
}

int GuessIsPerson(char *text) {
	char *w;

	w = strstr(text, " he ");
	if (w)
		return(1);
	w = strstr(text, "He ");
	if (w)
		return(1);
	w = strstr(text, "He's ");
	if (w)
		return(1);
	w = strstr(text, " he's ");
	if (w)
		return(1);
	w = strstr(text, " she ");
	if (w)
		return(1);
	w = strstr(text, "She ");
	if (w)
		return(1);
	w = strstr(text, " she's ");
	if (w)
		return(1);
	w = strstr(text, "She's ");
	if (w)
		return(1);
	w = strstr(text, " his ");
	if (w)
		return(1);
	w = strstr(text, "His ");
	if (w)
		return(1);
	w = strstr(text, " her ");
	if (w)
		return(1);
	w = strstr(text, "Her ");
	if (w)
		return(1);
	w = strstr(text, " this man ");
	if (w)
		return(1);
	w = strstr(text, "This man ");
	if (w)
		return(1);
	w = strstr(text, " this woman ");
	if (w)
		return(1);
	w = strstr(text, "This woman ");
	if (w)
		return(1);
	w = strstr(text, " I ");
	if (w)
		return(1);
	w = strstr(text, "I ");
	if (w)
		return(1);

	return(0);
}



/*** Implementation of the Jeopardy class ***/

Jeopardy::Jeopardy() {
	this->fGfxInit = false;
	this->screen = nullptr;
	this->clue_bmp = nullptr;
	this->ques_bmp = nullptr;
	this->w_x = 0;
	this->w_y = 0; 
	this->current_game_mode = mode_main_menu;
	this->game_type = GAME_JEOPARDY;
	this->rdown = false;
	this->round = 0;
	this->dd = nullptr;
	this->dd_bmp = nullptr;
	this->isgame = true;
	this->fasttimer = true;
	this->wager = -1;
	this->drawtimer = false;
	this->tcount = 0;
	this->timerlen = 0;
	this->introsound = false;
	this->boardsound = false;
	this->boardsound2 = false;
	this->response.ringin = 0;
	this->response.added = false;
}

Jeopardy::~Jeopardy() {
}

void Jeopardy::LoadSoundFx(void) {
	int fname_right[count_right_fx] = {
		4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 20, 23, 24, 25, 26, 27, 29, 30, 31, 32, 
		38, 41, 43, 44, 48, 50, 51, 52, 53, 54, 55, 56, 57, 64, 65, 68, 69, 70, 71, 72, 74, 76,
		77, 78, 79, 80, 84, 85, 88, 91, 92, 93, 94, 95, 96, 99,100,102,103,104,108,109,110,114,
		120,121,122,124,125,126,128,129,132,133,144,145,146,147,
	};
	int fname_wrong[count_wrong_fx] = {
		15, 16, 22, 37, 42, 47, 59, 63, 82, 83, 86, 87, 107, 123, 135,
	};
	int e;
	char buf[64];

	this->sound_fx[sfx_board].load_from_file("assets/board.wav");
	this->sound_fx[sfx_double1].load_from_file("assets/double.wav");
	this->sound_fx[sfx_double2].load_from_file("assets/alex-18.wav");
	this->sound_fx[sfx_double3].load_from_file("assets/alex-33.wav");
	this->sound_fx[sfx_double4].load_from_file("assets/alex-73.wav");
	this->sound_fx[sfx_double5].load_from_file("assets/alex-97.wav");
	this->sound_fx[sfx_double6].load_from_file("assets/alex-101.wav");
	this->sound_fx[sfx_double7].load_from_file("assets/alex-105.wav");
	this->sound_fx[sfx_double8].load_from_file("assets/alex-111.wav");
	this->sound_fx[sfx_double9].load_from_file("assets/alex-115.wav");
	this->sound_fx[sfx_double10].load_from_file("assets/alex-118.wav");
	this->sound_fx[sfx_double11].load_from_file("assets/alex-39.wav");
	this->sound_fx[sfx_double12].load_from_file("assets/alex-137.wav");
	this->sound_fx[sfx_double13].load_from_file("assets/alex-138.wav");
	this->sound_fx[sfx_double14].load_from_file("assets/alex-139.wav");
	this->sound_fx[sfx_double15].load_from_file("assets/alex-140.wav");
	this->sound_fx[sfx_double16].load_from_file("assets/alex-148.wav");
	this->sound_fx[sfx_reveal].load_from_file("assets/reveal.wav");
	this->sound_fx[sfx_round].load_from_file("assets/round.wav");
	this->sound_fx[sfx_time].load_from_file("assets/time.wav");
	this->sound_fx[sfx_think].load_from_file("assets/think.wav");
	this->sound_fx[sfx_think2].load_from_file("assets/think2.wav");
	this->sound_fx[sfx_think3].load_from_file("assets/think3.wav");
	this->sound_fx[sfx_round1intro].load_from_file("assets/alex-1.wav");
	this->sound_fx[sfx_round1introalt1].load_from_file("assets/alex-106.wav");
	this->sound_fx[sfx_round1introalt2].load_from_file("assets/alex-119.wav");
	this->sound_fx[sfx_round1introalt3].load_from_file("assets/alex-141.wav");
	this->sound_fx[sfx_round2intro].load_from_file("assets/alex-28.wav");
	this->sound_fx[sfx_round2introalt1].load_from_file("assets/alex-98.wav");
	this->sound_fx[sfx_round2introalt2].load_from_file("assets/alex-117.wav");
	this->sound_fx[sfx_round2introalt3].load_from_file("assets/alex-134.wav");
	this->sound_fx[sfx_round3intro].load_from_file("assets/alex-45.wav");
	this->sound_fx[sfx_boardstart1].load_from_file("assets/alex-3.wav");
	this->sound_fx[sfx_boardstart2].load_from_file("assets/alex-49.wav");
	this->sound_fx[sfx_boardstart3].load_from_file("assets/alex-58.wav");
	this->sound_fx[sfx_ddcorrect1].load_from_file("assets/alex-36.wav");
	this->sound_fx[sfx_ddcorrect2].load_from_file("assets/alex-40.wav");
	this->sound_fx[sfx_ddcorrect3].load_from_file("assets/alex-62.wav");
	this->sound_fx[sfx_ddcorrect4].load_from_file("assets/alex-81.wav");
	this->sound_fx[sfx_ddcorrect5].load_from_file("assets/alex-67.wav");
	this->sound_fx[sfx_ddcorrect6].load_from_file("assets/alex-113.wav");
	this->sound_fx[sfx_ddcorrect7].load_from_file("assets/alex-149.wav");
	this->sound_fx[sfx_wager1].load_from_file("assets/alex-34.wav");
	this->sound_fx[sfx_wager2].load_from_file("assets/alex-60.wav");
	this->sound_fx[sfx_wager3].load_from_file("assets/alex-19.wav");
	this->sound_fx[sfx_wager4].load_from_file("assets/alex-61.wav");
	this->sound_fx[sfx_wager5].load_from_file("assets/alex-75.wav");
	this->sound_fx[sfx_wager6].load_from_file("assets/alex-89.wav");
	this->sound_fx[sfx_wager7].load_from_file("assets/alex-90.wav");
	this->sound_fx[sfx_wager8].load_from_file("assets/alex-112.wav");
	this->sound_fx[sfx_wager9].load_from_file("assets/alex-116.wav");
	this->sound_fx[sfx_wager10].load_from_file("assets/alex-143.wav");
	this->sound_fx[sfx_selection].load_from_file("assets/alex-21.wav");
	this->sound_fx[sfx_gameintro1].load_from_file("assets/intro-1.wav");
	this->sound_fx[sfx_gameintro2].load_from_file("assets/intro-2.wav");
	this->sound_fx[sfx_gameintro3].load_from_file("assets/intro-3.wav");
	this->sound_fx[sfx_canruncategory].load_from_file("assets/alex-66.wav");

	this->sound_fx[sfx_playerchris].load_from_file("assets/player-chris.wav");
	this->sound_fx[sfx_playerjohn].load_from_file("assets/player-john.wav");
	this->sound_fx[sfx_playerbrad].load_from_file("assets/player-brad.wav");
	this->sound_fx[sfx_playerjonathan].load_from_file("assets/player-jonathan.wav");
	this->sound_fx[sfx_playerkay].load_from_file("assets/player-kay.wav");
	this->sound_fx[sfx_playermary].load_from_file("assets/player-mary.wav");
	this->sound_fx[sfx_playerjoshua].load_from_file("assets/player-joshua.wav");
	this->sound_fx[sfx_playeralex].load_from_file("assets/player-alex.wav");
	this->sound_fx[sfx_playerwillie].load_from_file("assets/player-willie.wav");
	this->sound_fx[sfx_playeromar].load_from_file("assets/player-omar.wav");
	this->sound_fx[sfx_playerlisa].load_from_file("assets/player-lisa.wav");
	this->sound_fx[sfx_playervan].load_from_file("assets/player-van.wav");
	this->sound_fx[sfx_playerwilliam].load_from_file("assets/player-william.wav");
	this->sound_fx[sfx_playerchristian].load_from_file("assets/player-christian.wav");
	this->sound_fx[sfx_playerkate].load_from_file("assets/player-kate.wav");
	this->sound_fx[sfx_playerbrandon].load_from_file("assets/player-brandon.wav");
	this->sound_fx[sfx_playerphil].load_from_file("assets/player-phil.wav");
	this->sound_fx[sfx_playerhillary].load_from_file("assets/player-hillary.wav");
	this->sound_fx[sfx_playercarl].load_from_file("assets/player-carl.wav");
	this->sound_fx[sfx_playercecilia].load_from_file("assets/player-cecilia.wav");
	this->sound_fx[sfx_playeralan].load_from_file("assets/player-alan.wav");
	this->sound_fx[sfx_playeranthony].load_from_file("assets/player-anthony.wav");
	this->sound_fx[sfx_playerjenny].load_from_file("assets/player-jenny.wav");
	this->sound_fx[sfx_playeradam].load_from_file("assets/player-adam.wav");
	this->sound_fx[sfx_playerben].load_from_file("assets/player-ben.wav");
	this->sound_fx[sfx_playerdavid].load_from_file("assets/player-david.wav");
	this->sound_fx[sfx_playererin].load_from_file("assets/player-erin.wav");
	this->sound_fx[sfx_playerlan].load_from_file("assets/player-lan.wav");
	this->sound_fx[sfx_playermark].load_from_file("assets/player-mark.wav");
	this->sound_fx[sfx_playermelinda].load_from_file("assets/player-melinda.wav");
	this->sound_fx[sfx_playermichael].load_from_file("assets/player-michael.wav");
	this->sound_fx[sfx_playerscott].load_from_file("assets/player-scott.wav");
	this->sound_fx[sfx_playerkaren].load_from_file("assets/player-karen.wav");
	this->sound_fx[sfx_playersusan].load_from_file("assets/player-susan.wav");
	this->sound_fx[sfx_playermitch].load_from_file("assets/player-mitch.wav");
	this->sound_fx[sfx_playerdamien].load_from_file("assets/player-damien.wav");
	this->sound_fx[sfx_playerrachel].load_from_file("assets/player-rachel.wav");
	this->sound_fx[sfx_playerjudy].load_from_file("assets/player-judy.wav");
	this->sound_fx[sfx_playerian].load_from_file("assets/player-ian.wav");
	this->sound_fx[sfx_playerpaul].load_from_file("assets/player-paul.wav");
	this->sound_fx[sfx_playersarah].load_from_file("assets/player-sarah.wav");
	this->sound_fx[sfx_playerjanet].load_from_file("assets/player-janet.wav");
	this->sound_fx[sfx_playerkathleen].load_from_file("assets/player-kathleen.wav");
	this->sound_fx[sfx_playeraric].load_from_file("assets/player-aric.wav");
	this->sound_fx[sfx_playervirginia].load_from_file("assets/player-virginia.wav");

	this->reading = NULL;

	//this->sound_fx[sfx_player].load_from_file("assets/player-.wav");

	for (e = 0; e < count_right_fx; ++e) {
		sprintf(buf, "assets/alex-%d.wav", fname_right[e]);
		this->right_fx[e].load_from_file(buf);
	}
	for (e = 0; e < count_wrong_fx; ++e) {
		sprintf(buf, "assets/alex-%d.wav", fname_wrong[e]);
		this->wrong_fx[e].load_from_file(buf);
	}
}

void Jeopardy::PrepareToAcceptInput() {
	if (!this->isgame)
		return;
	this->typebuffer[0] = 0;
	this->inputstart = false;
	this->showanswer = false;
	this->showamount = false;
	this->timesound = false;
	clback = this;
	if (this->current_game_mode != mode_wager && this->current_game_mode != mode_animating_clue && !this->clueshown && this->round < 2) {
		this->clueshown = true;
		this->tick_start = ticks;
	}
}

void Jeopardy::RoundBegin(void) {
	int cd;
	int e;
	int f;

	this->m_x = 0;
	this->m_y = 0;
	this->s_c = -1;
	this->s_a = -1;

	this->showamount = false;
	this->correctok = false;

	if (this->round == 0) {
		InitGameStats();
		this->SelectCategories();
		if (this->gamewon > 0 || this->game_type == GAME_JEOPARDY)
			this->control = 0;
		else
			this->control = randbetween(0, 1) + 1;
		this->pcoryat = 0;	// reset player's coryat score
	} else {
		if (this->game_type == GAME_JEOPARDY3) {
			// control of the board belongs to the player in third place
			if (this->playerscore < this->oppscore[0] && this->playerscore < this->oppscore[1])
				this->control = 0;
			else if (this->oppscore[0] < this->oppscore[1])
				this->control = 1;
			else
				this->control = 2;
		}
	}
 
	// place the DDs
	while (1) {
		this->dd_c[0] = randbetween(0, 5);
		this->dd_c[1] = randbetween(0, 5);
		if (dd_c[0] != dd_c[1])
			break;
	}
	for (e = 0; e < 2; ++e) {
		// chance of DD appearing at amount:
		//	0: 0%, 1: 1/23, 2: 6/23, 3: 8/23, 4: 8/23
		cd = randbetween(0, 22);
		switch (cd)
			{
		case 0:
			this->dd_a[e] = 1;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			this->dd_a[e] = 2;
			break;
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
			this->dd_a[e] = 3;
			break;
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
			this->dd_a[e] = 4;
			break;
			}
	}
	if (this->round < 1) {
		this->dd_c[1] = -1;
		this->dd_a[1] = -1;
	}

	for (e = 0; e < 6; ++e) {
		for (f = 0; f < 5; ++f) {
			this->revealed[e][f] = false;
			this->selected[e][f] = false;
		}
	}
	this->revealsound = false;

	this->clue_bmp = NULL;
	this->catreveal = -1;
	this->current_game_mode = mode_category_reveal;
	clback = this;
	this->tick_start = ticks;
	this->ddseen = false;
}

void Jeopardy::DDBitmap() {
	int nw;

	if (this->dd_bmp != nullptr) {
		delete dd_bmp;
		this->dd_bmp = nullptr;
	}

	dd_bmp = new SBitmap(w_x, w_y);
	dd_bmp->clear(BOARD_CLR);
	dd->blit(dd_bmp);
}

void Jeopardy::AnimateClue() {
	int th100;	// hundredths
	int x1, x2, y1, y2;
	int TOTAL;

	TOTAL = 80;
	_timerlock = true;

	this->showanswer = false;

	th100 = ticks - tick_start + 20;
	if (th100 > TOTAL)
		th100 = TOTAL;
	else
		this->clueshown = false;

	x1 = (cx1 * (TOTAL - th100)) / TOTAL;
	x2 = ((cx2 * (TOTAL - th100)) + ((this->w_x - 1) * th100)) / TOTAL;
	y1 = (cy1 * (TOTAL - th100)) / TOTAL;
	y2 = ((cy2 * (TOTAL - th100)) + ((this->w_y - 1) * th100)) / TOTAL;

	if (this->ddsel) {
		dd_bmp->stretch_blit(this->screen, 0, 0, this->dd_bmp->width(), this->dd_bmp->height(), x1, y1, (x2 - x1 + 1), (y2 - y1 + 1));
	} else {
		clue_bmp->stretch_blit(this->screen, 0, 0, this->clue_bmp->width(), this->clue_bmp->height(), x1, y1, (x2 - x1 + 1), (y2 - y1 + 1));
	}

	if (th100 == TOTAL) {
		ai_judge = -1;
		ai_judgeamt = 0;
		correctok = false;
		if (this->ddsel) {
			if (ticks - tick_start < 300)
				return;
			if (this->isgame) {
				if (this->game_type == GAME_JEOPARDY3 && this->control > 0) {
					this->current_game_mode = mode_opponent_dd_wager;
				} else {
					this->current_game_mode = mode_wager;
					response.ringin = 2;
				}
			} else {
				this->current_game_mode = mode_clue;
			}
			clback = NULL;
			this->tick_start = ticks;
			this->typebuffer[0] = 0;
			this->PrepareToAcceptInput();
			this->ForceRedraw();
		} else {
			this->typebuffer[0] = 0;
			this->current_game_mode = mode_clue;
			clback = NULL;
			this->tick_start = ticks;
			this->PrepareToAcceptInput();
		}
	}

	_timerlock = false;
}

int Jeopardy::FinalJeopardyWager(int ip) {
	int plc[3];
	int score[3];
	int wager[3];
	int e;
	int f;

	plc[0] = 0;
	plc[1] = 1;
	plc[2] = 2;
	score[0] = this->playerscore;
	score[1] = this->oppscore[0];
	score[2] = this->oppscore[1];

	for (e = 0; e < 3; ++e) {
StartAgain:
		for (f = e + 1; f < 3; ++f) {
			if (score[e] < score[f]) {
				int sw;
				sw = score[e];
				score[e] = score[f];
				score[f] = sw;
				sw = plc[e];
				plc[e] = plc[f];
				plc[f] = sw;
				goto StartAgain;
			}
		}
	}

	// first, determine the lead player's wager
	if (score[1] + score[1] <= score[0]) {
		// it's a runaway, lead player wagers maximum that guarantees a win
		wager[0] = score[0] - (score[1] + score[1]);
		if (wager[0] > 0)
			wager[0]--;
	} else {
		// lead player bids enough to take the lead in case second player doubles his or her score
		wager[0] = (score[1] + score[1]) - score[0];
		wager[0]++;
	}
	// now, the second place player's wager
	//  try to wager enough to win if the lead player gets it wrong
	wager[1] = ((score[0] - wager[0]) - score[1]) + 1;
	if (wager[1] > score[1]) {
		// it's a runaway: wager enough to maintain 2nd place
		wager[1] = score[1] - (score[2] + score[2]);
		wager[1]--;
		if (wager[1] < 0)
			wager[1] = 0;
	} else {
		// 40% chance we just bet it all, to ensure 1st place player doesn't low-bid
		if (RandPercent(40)) {
			wager[1] = score[1];
			if (RandPercent(30)) {
				// ... ok, maybe we hold back a buck
				--wager[1];
			}
		}
	}
	if (wager[1] < 0) {
		wager[1] = -wager[1];
		--wager[1];
	}
	// third place player's wager
	//  first, try to win
	wager[2] = ((score[0] - wager[0]) - score[2]) + 1;
	if (wager[2] > score[2]) {
		// can't win, try to get second place
		wager[2] = ((score[1] - wager[1]) - score[2]) + 1;
		if (wager[2] > score[2]) {
			// can't get second, just wager $0
			wager[2] = 0;
		}
	}
	if (wager[2] < 0) {
		wager[2] = -wager[2];
		wager[2]--;
	}

	for (e = 0; e < 3; ++e) {
		if (wager[e] > score[e])
			wager[e] = score[e];
		if (wager[e] < 0)
			wager[e] = 0;
		if (plc[e] == ip)
			return(wager[e]);
	}

	return(0);
}

void Jeopardy::PlayAlexResponse(bool right) {
	if (right) {
		if (this->ddsel) {
			// play a more enthusiastic response for getting a DD right
			int sid;

			forever {
				sid = sfx_ddcorrect1 + randbetween(0, 6);
				if (sid == sfx_ddcorrect7) {
					// too loud -- maybe edit this one? 
					continue;
				}
				if (sid == sfx_ddcorrect2 && this->game_type == GAME_JEOPARDY3) {
					// "Hey, you're in the lead!" only makes sense if you're leading:
					//   in fact, we'll only play this if the DD put you in the lead
					if (this->wasleading || !this->willlead)
						continue;
				}
				if (sid == sfx_ddcorrect6 && this->game_type == GAME_JEOPARDY3) {
					// "Your lead increaseth!"... only if you were in the lead
					if (!this->wasleading)
						continue;
				}
				break;
			}
			soundfx.play_sfx(this->sound_fx[sid]);
		} else {
			soundfx.play_sfx(this->right_fx[randbetween(0, count_right_fx - 1)]);
		}
	} else {
		soundfx.play_sfx(this->wrong_fx[randbetween(0, count_wrong_fx - 1)]);
	}
}

void Jeopardy::Initialize(const char* model_path, double tmp, bool twogpu, int screen_w, int screen_h) {
	int e;
	int f;

	std::cout << "*** Jeopardy! simulation by Christin Street (version 2.0) ***\n";
	std::cout << "Software copyright (c) 2011-2023 Chris Street\n";
	std::cout << "Now with artificially intelligent opponents (eat your heart out, IBM Watson!)\n\n";

	std::cout << "Loading Jeopardy! game assets...\n";
	this->category_font = new JeopardyFont("assets/swiss911.ttf");
	this->answer_font = new JeopardyFont("assets/korin.ttf");

	this->dollaramounts[0] = SBitmap::load_bmp("assets/200.bmp");
	this->dollaramounts[1] = SBitmap::load_bmp("assets/400.bmp");
	this->dollaramounts[2] = SBitmap::load_bmp("assets/600.bmp");
	this->dollaramounts[3] = SBitmap::load_bmp("assets/800.bmp");
	this->dollaramounts[4] = SBitmap::load_bmp("assets/1000.bmp");
	this->dollaramounts[5] = SBitmap::load_bmp("assets/400.bmp");
	this->dollaramounts[6] = SBitmap::load_bmp("assets/800.bmp");
	this->dollaramounts[7] = SBitmap::load_bmp("assets/1200.bmp");
	this->dollaramounts[8] = SBitmap::load_bmp("assets/1600.bmp");
	this->dollaramounts[9] = SBitmap::load_bmp("assets/2000.bmp");
	for (e = 0; e < 10; ++e)
		AdjBmp(this->dollaramounts[e]);

	this->dd = SBitmap::load_bmp("assets/dd.bmp");
	this->dd->alpha_opaque();
	this->LoadSoundFx();
	this->catdrawn = false;
	this->fGfxInit = true;

	std::cout << "Loading Jeopardy! clues...\n";
	sqlite3_open("assets/Jeopardy.db", &this->db);
	sqlite3_exec(this->db, "SELECT MAX(ID) FROM Category;", int_callback, NULL, NULL);
	this->max_catid = intRet_db;
	this->catstatus = new int [this->max_catid + 1];
	cluecount = new int [this->max_catid + 1];
	catarray = this->catstatus;
	cat_responses = new int [this->max_catid + 1];
	for (e = 0; e <= this->max_catid; ++e) {
		catarray[e] = 0;
		cluecount[e] = 0;
		cat_responses[e] = 0;
	}
	sqlite3_exec(this->db, "SELECT * FROM Category;", cat_callback1, NULL, NULL);
	sqlite3_exec(this->db, "SELECT * FROM Clue;", cat_callback2, NULL, NULL);
	sqlite3_exec(this->db, "SELECT * FROM QResponse;", cat_callback3, NULL, NULL);

	srand(time(NULL));

	this->gamewon = 0;
	this->winnings = 0;

	this->goup = false;
	this->judgedock = -1;

	lastcatsel = randbetween(0, 5);

	for (e = 0; e < 6; ++e)
		for (f = 0; f < 5; ++f)
		 selected[e][f] = false;

	this->coccup = 0;
	while (occupations[this->coccup] != NULL)
		this->coccup++;
	this->cfrom = 0;
	while (wherefrom[this->cfrom] != NULL)
		this->cfrom++;

	this->buzzload = false;
	this->reading = NULL;
	this->driftv = 5;
	this->delayv = 10;
	this->ai_judge = -1;
	this->ai_judgeamt = 0;
	this->correctok = false;

	Size(screen_w, screen_h);

	if (model_path != nullptr) {
#ifdef CODEHAPPY_CUDA
		bool four_bit = false;
		if (__stristr(model_path, "Q4") != nullptr) {
			four_bit = true;
		}
		std::string test_prompt = "Testing: one, two, three";
		std::vector<llama_token> toks;
		std::cout << "Warming up the large language model...\n";
		this->temp = tmp;
		llama = new Llama(model_path);
		llama->set_temp(temp);
		if (twogpu) {
			// the 70 billion paramater Llama 2 can fit entirely on two 24GB VRAM GPUs, if
			// it's quantized at 4 bit. 8 bit quantization, we can offload 22 layers/GPU.
			std::vector<float> split = { 1.0f, 1.0f };
			llama->layers_to_gpu(four_bit ? 999 : 44);
			llama->set_tensor_split(split);
		}
		llama->session_prompt(test_prompt);
		std::cout << test_prompt;
		llama->generate_tokens(toks, 32, true);
		std::cout << "\n\n****************************************\n\n";
#endif
	} else {
		llama = nullptr;
	}

	std::cout << "THIS... IS... JEOPARDY!\n";
}

char *catmap = NULL;
void Jeopardy::SaveCatmap(void) {
#ifndef CODEHAPPY_WASM
	FILE *ff;

	ff = fopen("catmap", "wb");
	fwrite(catmap, sizeof(char), this->max_catid + 1, ff);
	fclose(ff);
#endif	
}
void Jeopardy::LoadCatmap(void) {
#ifndef CODEHAPPY_WASM
	FILE *ff;
	int len;
	int e;

	catmap = new char [this->max_catid + 1];
	for (e = 0; e <= this->max_catid; ++e)
		catmap[e] = 0;

	len = filelen("catmap");

	ff = fopen("catmap", "rb");
	fread(catmap, sizeof(char), len, ff);
	fclose(ff);
#endif	
}

void CheckCatmap(int ncat) {
#ifndef CODEHAPPY_WASM
	int e;
	int c;

	for (e = 0, c = 0; e < ncat; ++e) {
		if (catmap[e] > 0)
			c++;
	}
	if (c + 100 > ncat) {
		for (e = 0; e < ncat; ++e)
			catmap[e] = 0;
	}
#endif	
}

void Jeopardy::SelectCategories(void) {
	// select random categories
	int e;
	int f;
	int catid;
	char *cmd;
	int err;
	int rd;
	int fail;
	ofstream catstats;

#ifndef CODEHAPPY_WASM
	if (catmap == NULL) {
		if (!FileExists("catmap")) {
			catmap = new char [this->max_catid + 1];
			for (e = 0; e <= this->max_catid; ++e)
				catmap[e] = 0;
			SaveCatmap();
		} else {
			LoadCatmap();
		}
	}

	CheckCatmap(this->max_catid + 1);

	{
	// output some statistics on categories
	int sj;
	int dj;
	int fj;
	int cm;
	catstats.open("catstats.txt");
	catstats << this->max_catid + 1 << " total categories" << endl;
	sj = 0;
	dj = 0;
	fj = 0;
	cm = 0;
	for (e = 0, f = 0; e <= this->max_catid; ++e) {
		if (this->catstatus[e] == 0)
			sj++;
		else if (this->catstatus[e] == 1)
			dj++;
		else if (this->catstatus[e] == 2)
			fj++;
		if (cluecount[e] == 5)
			cm++;
		if (catmap[e] > 0)
			f++;
	}
	catstats << sj << " categories in Jeopardy! round" << endl;
	catstats << dj << " categories in Double Jeopardy! round" << endl;
	catstats << fj << " Final Jeopardy! categories" << endl;
	catstats << cm << " total categories complete (with 5 clues)" << endl;
	catstats << f << " categories marked as played in the category map" << endl;
	catstats.close();
	catstats.clear();
	}
#else
	catmap = new char [this->max_catid + 1];
	for (e = 0; e <= this->max_catid; ++e)
		catmap[e] = 0;
#endif // CODEHAPPY_WASM

#ifdef CODEHAPPY_WASM
	if (false) {
#else
	if (FileExists("useboard.txt")) {
#endif	
		// use the category IDs in the file
		ifstream i;
		for (e = 0; e < 12; ++e)
			this->cats[e] = -1;
		i.open("useboard.txt");
		for (e = 0; e < 12; ++e) {
			i >> catid;
			if (i.eof())
				break;
			this->cats[e] = catid;
		}
		for (e = 0; e < 12; ++e) {
			if (e < 6)
				rd = 0;
			else
				rd = 1;
			if (this->cats[e] < 0) {
				forever {
					catid = randbetween(0, this->max_catid);
					if (this->catstatus[catid] < 0 || cluecount[catid] < 5)
						continue;
					if (this->catstatus[catid] != rd)
						continue;
					break;
				}
				this->cats[e] = catid;
				// we don't update the category map for these custom games.
			}
		}
		i.close();
		i.clear();
	} else {
		if (!this->game_specialty) {
			for (e = 0; e < 12; ++e) {
				if (e < 6)
					rd = 0;
				else
					rd = 1;
				fail = 0;
				forever {
					catid = randbetween(0, this->max_catid);
					if (this->catstatus[catid] < 0 || cluecount[catid] < 5)
						continue;
					// match rounds: show SJ clues in SJ and DJ clues in DJ 
					// (Double Jeopardy! clues are harder on average than Single Jeopardy! -- at least
					//   that's what Alex likes to say on the show) 
					if (this->catstatus[catid] != rd)
						continue;
				
					if (this->game_hard) {
						/* Category selection for a "hard" game. */
						if (catmap[catid] <= 0 && fail < 1000) {
							// we only want categories that we HAVE seen before
							fail++;
							continue;
						}
						if (cat_responses[catid] > 2 && fail < 1000) {
							// don't select categories that have many net correct answers
							fail++;
							continue;
						}
					} else {
						/* Category selection for a "regular" game. */

						// Don't repeat categories that have been played until every category has been shown.
						if (catmap[catid] > 0 && fail < 100) {
							fail++;
							continue;
						}
					}
					break;
				}
				for (f = 0; f < e; ++f) {
					if (this->cats[f] == catid)
						break;
				}
				if (f < e) {
					--e;
					continue;
				}
				this->cats[e] = catid;
				catmap[catid] = 1;
			}
		} else {
			// This is a specialty game. Select categories from the specialty array.
			// Ignores the category map: any category with the appropriate answer is fair game.
			for (e = 0; e < 12; ++e) {
				int ct;

				ct = randbetween(0, this->specialtycats.size() - 1);
				forever {
					for (f = 0; f < e; ++f) {
						if (this->cats[f] == this->specialtycats[ct])
							break;
					}
					if (f == e) {
						this->cats[e] = this->specialtycats[ct];
						break;
					}
				}
			}
		}
	}

	// FJ category
	forever {
		catid = randbetween(0, this->max_catid);
		if (this->catstatus[catid] != 2)
			continue;
		break;
	}

	cmd = sqlite3_mprintf(
		"SELECT Name FROM Category WHERE id=%d;",
		catid);
	err = sqlite3_exec(this->db, cmd, str_callback, NULL, NULL);
	sqlite3_free(cmd);

	this->final_cat = new char [strlen(strret) + 1];
	strcpy(this->final_cat, strret);

	cmd = sqlite3_mprintf(
		"SELECT Answer FROM Clue WHERE catid=%d;",
		catid);
	err = sqlite3_exec(this->db, cmd, str_callback, NULL, NULL);
	sqlite3_free(cmd);

	this->final_clue = new char [strlen(strret) + 1];
	strcpy(this->final_clue, strret);

	cmd = sqlite3_mprintf(
		"SELECT Question FROM Clue WHERE catid=%d;",
		catid);
	err = sqlite3_exec(this->db, cmd, str_callback, NULL, NULL);
	sqlite3_free(cmd);

	this->final_question = new char [strlen(strret) + 1];
	strcpy(this->final_question, strret);

	this->thinkmusic = false;
	this->fjdone = false;

	// other stuff to do at the start of a game
	this->playerscore = 0;
	this->oppscore[0] = 0;
	this->oppscore[1] = 0;
}

void Jeopardy::CreateContestants(void) {
	int e;

	this->playerid[0] = 0;
	this->playerid[1] = randbetween(0, NUM_ADDL_PLAYERS - 2) + 1;
	do {
		this->playerid[2] = randbetween(0, NUM_ADDL_PLAYERS - 2) + 1;
	} while (this->playerid[2] == this->playerid[1]);
	this->fjwager[0] = -1;
	this->fjwager[1] = -1;

	/*
			Create contestants with parameters indicating their likelihood to ring in and their likelihood
			of knowing the correct response.

			These parameters have been calculated to produce characteristics similar to the 
			actual game show. In particular:

				1. The proportion of "triple stumpers" is very similar to that observed on the show, not only
					per game, but per round and per clue value.
				2. Contestants are more likely to know the correct responses of the lower value clues than the
					higher value clues, and the probabilities correspond to those observed on the show.
				3. The combined Coryat scores of 3 computer generated players correspond almost exactly
					with the combined Coryat scores observed in actual games, in mean, median and
					standard deviation.
				4. Probabilities (parameters) are set per clue value and per Jeopardy! round, as the
					Double Jeopardy! round is on average slightly more difficult than the first Jeopardy!
					round, and high value clues are considerably more difficult than low value clues. 
								
			These parameters are set so that, in the first game, the computer players are about as difficult
			as "average" contestants on the show.

			Coryat has been quoted saying that 24,000 is an "average" score for a Jeopardy! contestant, however
			average Coryat scores around 21,000 produce the number of Triple Stumpers and the combined Coryat
			score ranges observed in actual games.

			As you win games, more difficult opponents are introduced into the mix to make long runs
			more challenging.
	*/

	// akp[]: the probability that the contestant "actually knows" the clue, IF they ring in (i.e.,
	// the probability that they "actually know", if they "think they know".)
	//
	// Based on data from JArchive, this is estimated as:
	//		akp[]: based on R/(W+R)
	// ...simply the number of right contestant responses given, divided by the total (right and wrong) contestant reponses
	// given.
	//
	// We're now doing this by dollar value, since there is a definite difference in akp[] between
	// the highest and lowest dollar value clues.
	//
	// Calculated from JArchive responses over many games (SJ and DJ round), Daily Double responses not
	// counted (since the players do not "ring in" on those clues.)
	//
	//   Here is the data:
	//
	//		1st row		349 R/20 W -- 94.58% correct
	//		2nd row		322 R/51 W -- 86.33% correct
	//		3rd row		308 R/43 W -- 87.75% correct
	//		4th row		269 R/39 W -- 87.34% correct
	//		5th row		257 R/56 W -- 82.11% correct 
	//
	for (e = 0; e < 2; ++e) {
		this->akp[e][0] = 93 + randbetween(0, 4);		// 93..97, median 95 (94.58%) 
		this->akp[e][1] = 82 + randbetween(0, 8);		// 82..90, median 86 (86.33%) 
		this->akp[e][2] = 84 + randbetween(0, 8);		// 84..92, median 88 (87.75%)
		this->akp[e][3] = 83 + randbetween(0, 8);		// 83..91, median 87 (87.34%)
		this->akp[e][4] = 78 + randbetween(0, 8);		// 78..86, median 82 (82.11%)

	// tkp[][]: the "thinks they know" probabilities that explain the number of "triple stumpers".
	// We keep separate values for separate dollar values, since high value clues are harder.
	//  If TS% is the proportion of triple stumpers, AKW is the probability that the contestant
	//  actually knows the answer, and TKW is the probability that the contestant thinks he or she
	//  knows the answer, then
	//              TKW = (1 - (TS%)^(1/3)) / AKW
	//  The percentage of "triple stumpers" is known, and the "actually known" percentage can
	//  be estimated based on R/(W+R), above. The data is, based on Double Jeopardy! round results
	//  since the value doubling in 2001 (to end-of-season 2010-2011):
	//		
	//		dollar value		# TS		# clues		TS %		TKW (using AKW% given above for row)
	//			$400			 512		11908		 4.30%		68.69%
	//			$800			 859		11471		 7.49%		67.01%
	//		  $1,200			1256		10729		11.71%		58.21%
	//		  $1,600			1744		10233		17.04%		51.02%
	//		  $2,000			3026		10613		28.51%		41.63%
	//
	//		Based on the first Jeopardy! round:
	//
	//		dollar value		# TS		# clues		TS %		TKW (using AKW% given above for row)
	//		    $200			 491		11992		 4.09%		69.29%
	//			$400			 886		11873		 7.46%		67.07%
	//			$600			1217		11465		10.61%		60.00%
	//		    $800			1600		11202		14.28%		54.65%
	//		  $1,000			2582		11132		23.19%		46.96%
	//
	//	Based on the above data, the first Jeopardy! round is slightly easier on average, with the third to
	//  fifth row receiving from 2 to 5 percentage point higher TKW scores. The two lowest value clues in
	//  each category, however, have almost identical TKW scores in the two rounds.
	//
	//  The slight difference in TKP scores between rounds is handled as a "fiddle factor" in tkpbonus().
	//
	// TODO: 
	//   Examine the assumption (implicit above) that "player A knows" and "player B knows" are independent;
	//   this is probably not quite correct (if player A and player B both don't know, it's probably less
	//   likely that player C knows. Some clues are just more obscure than others.)

#ifndef RINGIN_DEPENDENT
		this->tkp[e][0] = 64 + randbetween(0, 10);	// 64..74, median 69 (68.69%)
		this->tkp[e][1] = 63 + randbetween(0, 8);	// 63..71, median 67 (67.01%)
		this->tkp[e][2] = 54 + randbetween(0, 8);	// 54..62, median 58 (58.21%)
		this->tkp[e][3] = 48 + randbetween(0, 6);	// 48..54, median 51 (51.02%)
		this->tkp[e][4] = 39 + randbetween(0, 6);	// 39..45, median 42 (41.63%)
#else // RINGIN_DEPENDENT
		// for D = 80% (see explanation below)
		this->tkp[e][0] = 74 + randbetween(0, 8);	// 74..82, median 78 (78.33%)
		this->tkp[e][1] = 74 + randbetween(0, 6);	// 74..80, median 77 (76.66%)
		this->tkp[e][2] = 64 + randbetween(0, 6);	// 64..70, median 67 (66.74%)
		this->tkp[e][3] = 56 + randbetween(0, 6);	// 56..62, median 59 (58.58%)
		this->tkp[e][4] = 45 + randbetween(0, 6);	// 45..51, median 48 (47.89%)
#endif

		// the players get harder, on average, as you win more games...
		if (this->gamewon > 0) {
			int f;
			for (f = 0; f < 5; ++f) {
				this->tkp[e][f] += randbetween(0, (this->gamewon * 5) - 1);
				if (this->tkp[e][f] > 90)
					this->tkp[e][f] = 90;
			}
		}

		/*

			An examination of "player A knows" and "player B knows" being dependent events:

			Let TS% be the proportion of "triple stumpers", K be the probability of a contestant
			knowing the answer (this is TKW times AKW above), and D be a dependency factor.
			For example, if we think that player B is 90% as likely to know the answer if player
			A doesn't, then D = 0.9.

			Then the expression for TS% is:

			   TS% = (1 - K) (1 - D*K)^2

			   or
									    2	    2   2 3
			   TS% = 1 + (-2D -1) K + (D + 2D) K - D K

		   With D = 100% (independent events), the above becomes the same equation as used in the independent
		   model.

		   With D = 90%, the cubic equation to solve for TS% is

		      TS% = 1 - 2.8 K + 2.61 K^2 - 0.81 K^3

		   For D = 80%:

		      TS% = 1 - 2.6 K + 2.24 K^2 - 0.64 K^3

		   For D = 70%:

		      TS% = 1 - 2.4 K + 1.89 K^2 - 0.49 K^3

		  Using D = 90%, and solving for K = (TKW * AKW), we get (using Double J! figures shown above)

						TS%			K		AKW (above)		TKW
		  1st row		 4.30%		69.44%	94.58%			73.42%
		  2nd row		 7.49%		61.87%	86.33%			71.67%
		  3rd row		11.71%		54.65%	87.75%			62.28%
		  4th row		17.04%		47.69%	87.34%			54.60%
		  5th row		28.51%		36.60%	82.11%			44.57%

		  These are on average about 3-5 percentage points above the figures shown above, which will
		  give higher individual Coryat scores, although since the dependent algorithm has to be used 
		  while ringing in, this doesn't necessarily mean a higher combined Coryat score for a 
		  3-contestant game.

		  Same, for D = 80%: 

						TS%			K		AKW (above)		TKW
		  1st row		 4.30%		74.08%	94.58%			78.33%
		  2nd row		 7.49%		66.18%	86.33%			76.66%
		  3rd row		11.71%		58.56%	87.75%			66.74%
		  4th row		17.04%		51.16%	87.34%			58.58%
		  5th row		28.51%		39.32%	82.11%			47.89%

		*/
		this->acoryat[e] = CalculateContestantAverageCoryat(e);
		if (this->acoryat[e] < 20000)
			--e;		// ensure the minimum Coryat allowed is $20K
	}


	this->ioccup[0] = randbetween(0, this->coccup - 1);
	do {
		this->ioccup[1] = randbetween(0, this->coccup - 1);;
	} while (this->ioccup[0] == this->ioccup[1]);
	this->ifrom[1] = randbetween(0, this->cfrom - 1);
	do {
		this->ifrom[0] = randbetween(0, this->cfrom - 1);
	} while (this->ifrom[0] == this->ifrom[1]);
}

void Jeopardy::ForceRedraw(bool all) {
	if (all)
		this->catdrawn = false;
}


void Jeopardy::DrawBoard(void)
{
	char *cmd;
	int err;
	int e;
	int x;
	int y;

	if (!this->catdrawn) {
		// Jeopardy! board blue
		screen->clear(BOARD_CLR);
	}

	// boxes
	for (e = 0; e < 7; ++e) {
		y = e * this->w_y / 6;
		y -= BOARDTHICK;
		screen->rect_fill(0, y, this->w_x - 1, y + BOARDTHICK * 2, BLACK_CLR);
	}
	for (e = 0; e < 7; ++e) {
		x = e * this->w_x / 6;
		x -= BOARDTHICK;
		screen->rect_fill(x, 0, x + BOARDTHICK * 2, this->w_y - 1, BLACK_CLR);
	}

	if (!this->catdrawn) {
		// print the category names
		for (e = 0; e < 6; ++ e) {
			SBitmap *catbmp;
			int x2;
			int y2;
			int ys;
			int yp;
			int nw;
			int f;

			for (f = 0; f < 5; ++f)
				if (!this->selected[e][f])
					break;

			if (f == 5)
				continue;		// category's spent, OK

			x = e * this->w_x / 6;
			x2 = (e + 1) * this->w_x / 6;
			x += BOARDTHICK;
			x2 -= BOARDTHICK;
			y = BOARDTHICK;
			y2 = this->w_y / 6 - BOARDTHICK;

			x += CATPADDING;
			x2 -= CATPADDING;
			
			cmd = sqlite3_mprintf(
				"SELECT Name FROM Category WHERE id=%d;",
				this->cats[e + this->round * 6]);
			err = sqlite3_exec(this->db, cmd, str_callback, NULL, NULL);
			sqlite3_free(cmd);

			if (strlen(strret) < 8) {
				int w;
				w = (x2 - x) / 4;
				x += w;
				x2 -= w;
			}

			category_names[e] = strret;
			catbmp = this->category_font->RenderCategory(strret);

			nw = (x2 - x + 1) * 9 / 10;
			ys = catbmp->height() * nw / catbmp->width();
			while (ys > (y2 - y) && ys > 1) {
				ys *= 4;
				ys /= 5;
			}
			yp = (y + y2) / 2 - ys / 2;

			catbmp->stretch_blit(this->screen, 0, 0, catbmp->width(), catbmp->height(), (x + x2 - nw) / 2, yp, nw, ys);
			delete catbmp;
		}
	}

	if (this->current_game_mode == mode_board_reveal && !this->revealsound) {
		this->revealsound = true;
		soundfx.play_sfx(this->sound_fx[sfx_board]);
	}

	if (!this->catdrawn || this->current_game_mode == mode_board_reveal) {
		// print dollar amounts on the board

		for (x = 0; x < 6; ++x) {
			for (y = 0; y < 5; ++y) {
				int x1, x2, y1, y2;
				int w;

				if (this->selected[x][y])
					continue;

				if (!this->revealed[x][y])
					continue;

				x1 = x * this->w_x / 6;
				x2 = (x + 1) * this->w_x / 6;
				x1 += BOARDTHICK;
				x2 -= BOARDTHICK;
				y1 = (y + 1) * this->w_y / 6;
				y2 = (y + 2) * this->w_y / 6;
				y1 += BOARDTHICK;
				y2 -= BOARDTHICK;

				w = (x2 - x1 + 1) * (y2 - y1 + 1) / this->dollaramounts[y + (5 * this->round)]->height();

				x1 += ((x2 - x1 + 1) - w) / 2;

				SBitmap* da = this->dollaramounts[y + (5 * this->round)];
				da->stretch_blit(this->screen, 0, 0, da->width(), da->height(), x1, y1, w, (y2 - y1 + 1));
			}
		}
	}

	this->catdrawn = true;

	// highlight selection, if any
	for (x = 0; x < 6; ++x) {
		for (y = 1; y < 6; ++y) {
			int x1, x2, y1, y2;

			x1 = x * this->w_x / 6;
			x2 = (x + 1) * this->w_x / 6;
			x1 += BOARDTHICK;
			x2 -= BOARDTHICK;
			y1 = y * this->w_y / 6;
			y2 = (y + 1) * this->w_y / 6;
			y1 += BOARDTHICK;
			y2 -= BOARDTHICK;

			if (this->m_x >= x1 && this->m_x <= x2 &&
				this->m_y >= y1 && this->m_y <= y2 &&
				!this->selected[x][y-1]) {
				this->s_c = x;
				this->s_a = y - 1;
				break;
			}
		}
	}

	if (this->s_c > -1) {
		char buf[256];
		int x1, x2, y1, y2;
		x1 = this->s_c * this->w_x / 6;
		x2 = (this->s_c + 1) * this->w_x / 6;
		y1 = (this->s_a + 1) * this->w_y / 6;
		y2 = (this->s_a + 2) * this->w_y / 6;

		screen->rect(x1, y1, x2, y2, WHITE_CLR);
		screen->rect(x1+1, y1+1, x2-1, y2-1, WHITE_CLR);

		if (this->s_c != this->s_cl || this->s_a != this->s_al)
			this->ForceRedraw();
		this->s_cl = this->s_c;
		this->s_al = this->s_a;
	}
}

void Jeopardy::DrawTypeBuffer(void) {
	int x;
	int y;
	SBitmap *txt;

	txt = this->category_font->RenderWord(this->typebuffer, true);

	y = this->screen->height() * 7 / 8;
	y -= 30;

	if (txt->width() >= screen->width()) {
		txt->blit(txt->width() - this->w_x, 0, txt->width() - 1, txt->height() - 1, screen, 0, y);
	} else {
		x = (this->screen->width() - txt->width()) / 2;
		txt->blit(this->screen, 0, 0, x, y, txt->width(), txt->height());
	}
	//delete txt;

	if (this->drawtimer) {
		int c;
		if (this->timerlen > (TIMERLENGTH / 2))
			c = WHITE_CLR;
		else {
			// fade to red
			int cc = 0xff * this->timerlen;
			cc /= (TIMERLENGTH / 2);
			cc &= 0xff;
			c = RGB_NO_CHECK(0xff, cc, cc);
		}
		screen->rect_fill((screen->width() - timerlen) / 2, y + 70, (screen->width() + this->timerlen) / 2, y + 70 + TIMERHEIGHT, c);
	}
}

void Jeopardy::DrawAITypeBuffer(void) {
	int x;
	int y;
	SBitmap *txt;
	std::string input = current_generation_str();
	if (input.empty())
		return;

	txt = this->category_font->RenderWord(input.c_str(), true);

	y = this->screen->height() * 7 / 8;
	y -= 30;

	if (txt->width() >= screen->width()) {
		txt->blit(txt->width() - this->w_x, 0, txt->width() - 1, txt->height() - 1, screen, 0, y);
	} else {
		x = (this->screen->width() - txt->width()) / 2;
		txt->blit(this->screen, 0, 0, x, y, txt->width(), txt->height());
	}
	//delete txt;
}

char __dstr[32];
static const char *DollarStr(int am) {
	char add[32];

	if (am < 0) {
		strcpy(__dstr, "-$");
		am = -am;
	} else {
		strcpy(__dstr, "$");
	}
	if (am >= 1000) {
		int t;
		t = am / 1000;
		sprintf(add, "%d,", t);
		strcat(__dstr, add);
		am -= t * 1000;
	}
	sprintf(add, "%03d", am);
	strcat(__dstr, add);

	return(__dstr);
}


void Jeopardy::DrawScore(void) {
	char buf[32];
	char add[32];
	int am;
	SBitmap *amount;
	int x;
	int y;
	int e;

	if (this->game_type == GAME_JEOPARDY) {
		strcpy(buf, DollarStr(this->playerscore));
		strcat(buf, " / ");
		strcat(buf, DollarStr(this->pcoryat));

		amount = this->category_font->RenderWord(buf);

		y = this->screen->height() * 7 / 8;
		y -= 30;

		x = (this->screen->width() - amount->width()) / 2;

		amount->blit(this->screen, 0, 0, x, y, amount->width(), amount->height());
		delete amount;
	} else if (this->game_type == GAME_JEOPARDY3) {
		for (e = 0; e < 3; ++e) {
			int id;

			if (e == 0)
				am = this->playerscore;
			else
				am = this->oppscore[e - 1];
			id = this->playerid[e];
			strcpy(buf, DollarStr(am));

			amount = this->category_font->RenderWord(buf);

			y = this->screen->height() * 7 / 8;
			y -= 30;

			x = ((this->screen->width() / 4) * (e + 1));

			if (this->round == 2 || this->ddsel)
				y -= 60;

			amount->blit(this->screen, 0, 0, x, y, amount->width(), amount->height());
			delete amount;

			amount = this->category_font->RenderWord((char *)playernames[id]);
			y -= (amount->height() + 6);
			amount->blit(this->screen, 0, 0, x, y, amount->width(), amount->height());
			delete amount;
		}
	}
}

void Jeopardy::DrawWager(void) {
	char buf[256];
	char add[32];
	int x;
	int y;
	int am;
	SBitmap *txt;

	screen->clear(BOARD_CLR);

	if (this->round == 2 && this->playerscore <= 0) {
		strcpy(buf, "YOU HAVE NO MONEY TO WAGER IN FINAL JEOPARDY!");
		y = this->w_y / 4 - 30;
		DrawCenteredText(buf, y);

		strcpy(buf, "HOWEVER, WE WILL LET YOU PARTICIPATE ANYWAY, WITH NO MONEY AT RISK.");
		y += 80;
		DrawCenteredText(buf, y);

		strcpy(buf, "PLEASE PRESS \"ENTER\" TO CONTINUE");
		y += 80;
		DrawCenteredText(buf, y);

		return;
	}

	if (this->round == 2) {
		strcpy(buf, "PLEASE ENTER YOUR FINAL JEOPARDY! WAGER.");
		this->judgedock = -1;
	} else {
		strcpy(buf, "PLEASE ENTER YOUR DAILY DOUBLE WAGER.");
	}

	y = this->w_y / 4 - 30;
	DrawCenteredText(buf, y);

	strcpy(buf, "YOU HAVE ");
	am = this->playerscore;
	strcat(buf, DollarStr(am));

	if (this->game_type == GAME_JEOPARDY3) {
		int e;
		int f;

		this->DrawScore();
		if (this->round == 2 && this->fjwager[0] < 0) {
			this->fjwager[0] = this->FinalJeopardyWager(1);
			this->fjwager[1] = this->FinalJeopardyWager(2);
			// Final Jeopardy! right 57% of the time...
			this->fjright[0] = RandPercent(57);
			this->fjright[1] = RandPercent(57);
			this->fjgen[0] = false;
			this->fjgen[1] = false;
		}

		plc[0] = 0;
		plc[1] = 1;
		plc[2] = 2;
		fjs[0] = this->playerscore;
		fjs[1] = this->oppscore[0];
		fjs[2] = this->oppscore[1];
		for (e = 0; e < 3; ++e) {
StartAgain:
			for (f = e + 1; f < 3; ++f) {
				if (fjs[e] < fjs[f]) {
					int sw;
					sw = fjs[e];
					fjs[e] = fjs[f];					
					fjs[f] = sw;
					sw = plc[e];
					plc[e] = plc[f];
					plc[f] = sw;
					goto StartAgain;
				}
			}
		}
	}

	y = this->w_y / 3 - 30;
	DrawCenteredText(buf, y);

	if (this->round == 0 && this->playerscore < 1000 ||
		this->round == 1 && this->playerscore < 2000) {
		if (this->round == 0)
			strcpy(buf, "YOU MAY WAGER UP TO $1,000.");
		else
			strcpy(buf, "YOU MAY WAGER UP TO $2,000.");
		y += 80;
		DrawCenteredText(buf, y);
	}

	this->DrawTypeBuffer();
}

void was_lead(bool& wasleading, int score, int opp1, int opp2) {
	wasleading = (score >= opp1) && (score >= opp2);
}

void was_will_lead(bool& wasleading, bool& willlead, int score, int wager, int opp1, int opp2) {
	wasleading = (score >= opp1) && (score >= opp2);
	willlead = ((score + wager) >= opp1) && ((score + wager) >= opp2);
}

bool Jeopardy::ValidateWager(void) {
	int w;
	char *b;

	if (this->game_type == GAME_JEOPARDY3 && this->round != 2 && this->control > 0)
		return(false);

	wasleading = false;
	willlead = false;
	istruedd = false;

	if (this->round == 2 && this->playerscore <= 0) {
		// in this version, we'll let you play FJ!, but not wager anything if you are in the red.
		this->wager = 0;
		return(true);
	}

	b = this->typebuffer;
	if (*b == '$')
		++b;	// leading dollar sign is certainly O.K.

	w = atoi(b);
	was_will_lead(wasleading, willlead, playerscore, w, oppscore[0], oppscore[1]);

	if (w == this->playerscore)
		istruedd = true;

	if (w < 0)
		return(false);
	
	if (this->playerscore > 0 && w == 0 && this->round < 2)
		return(false);

	// check maximum DD wagers
	if (this->round == 0 && this->playerscore < 1000 && w <= 1000) {
		this->wager = w;
		return(true);
	}
	if (this->round == 1 && this->playerscore < 2000 && w <= 2000) {
		this->wager = w;
		return(true);
	}

	if (w <= this->playerscore) {
		this->wager = w;
		return(true);
	}
	return(false);
}

void Jeopardy::DrawBuzzerResults(void)
{
	SBitmap *txt;
	int x;
	int y;
	char str[256];
	int e;
	int c;

	screen->clear(BOARD_CLR);

	e = 0;
	c = 10;

	/* put a nice graph here? */ 

	sprintf(str, "ATTEMPTED %d OF %d CLUES", buzzsc.nattempt, buzzsc.ntotal);
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	sprintf(str, "CALLED ON %d OF %d ATTEMPTS (%2d%%)", buzzsc.ncall, buzzsc.nattempt, (buzzsc.ncall * 100 / buzzsc.nattempt));
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	sprintf(str, "EARLY ON %d OF %d ATTEMPTS (%2d%%)", buzzsc.nearly, buzzsc.nattempt, (buzzsc.nearly * 100 / buzzsc.nattempt));
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	sprintf(str, "<= 5/100THS SECOND ON %d (%2d%%)", buzzsc.n5, (buzzsc.n5 * 100 / buzzsc.nattempt));
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	sprintf(str, "<= 10/100THS SECOND ON %d (%2d%%)", buzzsc.n10, (buzzsc.n10 * 100 / buzzsc.nattempt));
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	sprintf(str, "<= 15/100THS SECOND ON %d (%2d%%)", buzzsc.n15, (buzzsc.n15 * 100 / buzzsc.nattempt));
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	sprintf(str, "AVG. ACTIVATION LIGHT DELAY %d/100THS SEC.", buzzsc.tdelay / buzzsc.ntotal); 
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	strcpy(str, "PRESS THE LEFT MOUSE BUTTON TO CONTINUE");
	e++;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);
}


void Jeopardy::DrawBuzzerDelayParam(void) {
	SBitmap *txt;
	int x;
	int y;
	char str[256];
	int e;
	int c;

	screen->clear(BOARD_CLR);

	c = 7;

	strcpy(str, "THE DELAY BEFORE THE BUZZERS ARE ACTIVATED WILL");
	e = 0;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	sprintf(str, "FLUCTUATE. IT WILL BE WITHIN %d/100THS OF A SECOND", this->delayv);
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);
	
	strcpy(str, "OF WHEN ALEX FINISHES SPEAKING (BEFORE OR AFTER)");
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	sprintf(str, "IT MAY DRIFT UP TO %d/100THS OF A SECOND", this->driftv);
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	strcpy(str, "PRESS + OR - TO ADJUST DELAY PARAMETER");
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	strcpy(str, "PRESS < OR > TO ADJUST DRIFT PARAMETER");
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);

	strcpy(str, "PRESS LEFT MOUSE BUTTON TO CONTINUE");
	++e;
	y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
	DrawCenteredText(str, y);
}


void Jeopardy::DrawInstructions(void)
{
	SBitmap *txt;
	int x;
	int y;
	int e;
	const char *ins[4][10] = {
		{
			"THIS IS A 50 QUESTION TEST MODELLED AFTER THE",
			"JEOPARDY! CONTESTANT QUALIFICATION EXAM.",
			"YOU WILL BE SHOWN THE CATEGORY AND THEN THE CLUE.",
			"35 CORRECT IS A PASSING SCORE.",
			"YOU DO NOT HAVE TO PHRASE YOUR RESPONSES",
			"IN THE FORM OF A QUESTION, HOWEVER YOU MUST ENTER",
			"EACH RESPONSE BEFORE THE TIMER RUNS OUT.",
			"YOU MAY NOT RETURN TO AN UNANSWERED QUESTION.",
			"PRESS THE LEFT MOUSE BUTTON TO BEGIN",
			NULL
		},
		{
			"PRESS THE LEFT MOUSE BUTTON TO BEGIN",
			NULL
		},
		{
			NULL
		},
		{
			"THIS IS A SIMULATION OF THE JEOPARDY! BUZZER",
			"ALEX WILL READ A CLUE TO YOU",
			"AFTER HE FINISHES SPEAKING, THE BUZZER WILL BE",
			"ACTIVATED AND LIGHTS WILL BE SHOWN ON THE SIDES",
			"OF THE SCREEN - PRESS THE SPACE BAR TO BUZZ IN",
			"IF YOU BUZZ IN TOO EARLY YOU WILL BE LOCKED OUT",
			"YOUR REACTION TIME WILL BE SHOWN",
			"THE BUZZER ACTIVATION DELAY MAY VARY",
			"PRESS THE LEFT MOUSE BUTTON TO BEGIN",
			NULL
		},
	};
	int c;

	for (c = 0; ins[this->game_type - 1][c]; ++c)
		;

	screen->clear(BOARD_CLR);
	for (e = 0; e < c; ++e) {
		char str[256];
		if (this->game_type == GAME_BUZZER && this->buzzerscore >= 0 && e == 6) {
			strcpy(str, "ADJUST YOUR TIMING AS NECESSARY ");
		} else	{
			strcpy(str, ins[this->game_type - 1][e]);
		}
		y = ((this->w_y * (e + 1)) / (c + 1)) - 30;
		DrawCenteredText(str, y);
	}
}

SBitmap* text_label_bitmap(JeopardyFont* f, int idx) {
	const char* texts[] = {
		"PLAY JEOPARDY! (SOLO)",
		"PLAY JEOPARDY! (WITH OPPONENTS)",
#ifndef CODEHAPPY_WASM		
		"TAKE 50 QUESTION CONTESTANT EXAM",
		"TAKE THE TRIPLE STUMPER CHALLENGE",
		"PRACTICE BUZZER SIMULATION",
		"PRACTICE BUZZER SIMULATION (#2)",
		"PLAY A GAME OF JEOPARDY! (SOLO HARD!)",
		"PLAY JEOPARDY! WITH SPECIALTY CATEGORY",
#endif		
	};
	static SBitmap* text_bmps[20] = { nullptr };

	if (text_bmps[idx] != nullptr)
		return text_bmps[idx];

	text_bmps[idx] = f->RenderWord(texts[idx]);
	return text_bmps[idx];
}


void Jeopardy::DrawMainMenu(void) {
	SBitmap *txt;
	int x;
	int e;

	screen->clear(BOARD_CLR);

	txt = text_label_bitmap(this->category_font, 0);
	opy[0] = this->w_y / (MM_OPTIONS + 1) - 30;
	x = (this->w_x - txt->width()) / 2;
	minx = x;
	txt->blit(screen, 0, 0, x, opy[0], txt->width(), txt->height());

	txt = text_label_bitmap(this->category_font, 1);
	opy[1] = (this->w_y * 2) / (MM_OPTIONS + 1) - 30;
	x = (this->w_x - txt->width()) / 2;
	if (x < minx)
		minx = x;
	txt->blit(screen, 0, 0, x, opy[1], txt->width(), txt->height());

#ifndef CODEHAPPY_WASM
	txt = text_label_bitmap(this->category_font, 2);
	opy[2] = (this->w_y * 3) / (MM_OPTIONS + 1) - 30;
	x = (this->w_x - txt->width()) / 2;
	if (x < minx)
		minx = x;
	txt->blit(screen, 0, 0, x, opy[2], txt->width(), txt->height());

	txt = text_label_bitmap(this->category_font, 3);
	opy[3] = (this->w_y * 4) / (MM_OPTIONS + 1) - 30;
	x = (this->w_x - txt->width()) / 2;
	if (x < minx)
		minx = x;
	txt->blit(screen, 0, 0, x, opy[3], txt->width(), txt->height());

	txt = text_label_bitmap(this->category_font, 4);
	opy[4] = (this->w_y * 5) / (MM_OPTIONS + 1) - 30;
	x = (this->w_x - txt->width()) / 2;
	if (x < minx)
		minx = x;
	txt->blit(screen, 0, 0, x, opy[4], txt->width(), txt->height());

	txt = text_label_bitmap(this->category_font, 5);
	opy[5] = (this->w_y * 6) / (MM_OPTIONS + 1) - 30;
	x = (this->w_x - txt->width()) / 2;
	if (x < minx)
		minx = x;
	txt->blit(screen, 0, 0, x, opy[5], txt->width(), txt->height());

	txt = text_label_bitmap(this->category_font, 6);
	opy[6] = (this->w_y * 7)  / (MM_OPTIONS + 1) - 30;
	x = (this->w_x - txt->width()) / 2;
	minx = x;
	txt->blit(screen, 0, 0, x, opy[6], txt->width(), txt->height());

	txt = text_label_bitmap(this->category_font, 7);
	opy[7] = (this->w_y * 8)  / (MM_OPTIONS + 1) - 30;
	x = (this->w_x - txt->width()) / 2;
	minx = x;
	txt->blit(screen, 0, 0, x, opy[7], txt->width(), txt->height());
#endif	

	minx -= 80;
	if (minx < 0)
		minx = 0;
	this->mmsel = -1;
	this->game_hard = false;
	if (this->m_x >= minx && this->m_x <= this->w_x - minx) {
		for (e = 0; e < MM_OPTIONS; ++e) {
			if (this->m_y >= opy[e] - 30 && this->m_y <= opy[e] + 90) {
				this->mmsel = e;
				screen->rect(minx, opy[e] - 30, this->w_x - minx, opy[e] + 90, WHITE_CLR);
				screen->rect(minx + 1, opy[e] - 30 + 1, this->w_x - minx - 1, opy[e] + 90 - 1, WHITE_CLR);
				break;
			}
		}
	}
}


void Jeopardy::DrawExamStatus(void) {
	char buf[256];
	SBitmap *txt;

	sprintf(buf, "QUESTION %d/50 (%d RIGHT)", this->nquestions, this->nright);
	txt = this->category_font->RenderWord(buf);
	txt->blit(this->screen, 0, 0, 10, 10, txt->width(), txt->height());
	delete txt;
}


int Jeopardy::OpponentDDWager(void) {
	int am;
	int leadscore;
	int score;

	leadscore = this->playerscore;
	if (this->oppscore[0] > leadscore)
		leadscore = this->oppscore[0];
	if (this->oppscore[1] > leadscore)
		leadscore = this->oppscore[1];

	// true DD under $5000, $5000 after that
	am = this->oppscore[this->control - 1];
	score = am;
	if (am < 1000)
		am = 1000;
	else if (am < 5000)
		;
	else
		am = 5000;

	if (RandPercent(25))
		// sometimes a player is just timid...
		am /= 2;

	if (RandPercent(30) && leadscore == score)
		// ...especially if they are in the lead
		am /= 2;

	if (RandPercent(20)) {
		// and then there's the seemingly random wagers that sometimes pop up...
		int rw;

		rw = 500 + randbetween(0, 19) * 100;
		if (rw < am)
			am = rw;
	}

	// in the Double Jeopardy! round, players might be more aggressive if they are getting shut out...
	if (this->round == 1) {
		if (leadscore > (score + am) * 2 && score > 0) {
			// wager is too low, let's bump it
			am = (leadscore / 2) - score;
			if ((score + am) * 2 < leadscore)
				am++;

			// sometimes the player will want to be a little extra aggressive
			if (RandPercent(25))
				am += 1000;

			if (am > score)
				am = score;	// a true DD after all...
		}
		// ...Or they might want to try and put themselves out of reach in FJ
		else if (score == leadscore && score > 0 && RandPercent(40)) {
			int score2;

			score2 = this->playerscore;
			if (this->oppscore[0] != score && this->oppscore[0] > score2)
				score2 = this->oppscore[0];
			if (this->oppscore[1] != score && this->oppscore[1] > score2)
				score2 = this->oppscore[1];

			if (leadscore < score2 * 2) {
				am = (score2 * 2) - leadscore;
			}
			if (leadscore > score2 * 2 && leadscore - am <= score2 * 2) {
				// The score leader definitely doesn't want to give up a lock in the DJ! round 
				am = leadscore - (score2 * 2);
				am /= 2;
			}
		}
		if (am < 1)
			am = 1;
	}

	return(am);
}


void Jeopardy::DrawDDWager(void) {
	char buf[128];
	SBitmap *txt;
	int x;
	int y;
	const char *pn;
	int w;

	screen->clear(BOARD_CLR);

	pn = playernames[this->playerid[this->control]];

	wasleading = false;
	willlead = false;
	switch (control) {
	case 1:
		was_will_lead(wasleading, willlead, oppscore[0], ddwagers, playerscore, oppscore[1]);
		break;
	case 2:
		was_will_lead(wasleading, willlead, oppscore[1], ddwagers, playerscore, oppscore[0]);
	}

	istruedd = false;
	if (this->ddwagers == this->oppscore[this->control - 1])
		istruedd = true;

	strcpy(buf, "YOUR OPPONENT HAS FOUND A DAILY DOUBLE");
	y = 90;
	DrawCenteredText(buf, y);

	w = this->ddwagers;
	sprintf(buf, "%s BIDS $%d", pn, w);
	y += 90;
	DrawCenteredText(buf, y);

	y += 90;
	strcpy(buf, "PRESS A KEY TO CONTINUE");
	DrawCenteredText(buf, y);

	this->DrawScore();
}

void Jeopardy::DrawGameEnd(void) {
	char winstr[256];
	char buf[256];
	char add[256];
	SBitmap *txt;
	int y;
	int x;
	int am;

	y = 70;
	am = this->winnings;

	this->screen->clear(BOARD_CLR);

	switch (this->place) {
	case 1:
		strcpy(buf, "CONGRATULATIONS!");
		DrawCenteredText(buf, y);
		y += 90;

		if (this->gamewon == 1)
			strcpy(buf, "YOU ARE THE NEW JEOPARDY! CHAMPION");
		else
			strcpy(buf, "YOU REMAIN THE JEOPARDY! CHAMPION");
		DrawCenteredText(buf, y);

		sprintf(buf, "YOUR %d-DAY TOTAL WINNINGS ARE ", this->gamewon);
		y += 90;
		break;
	case 2:
		strcpy(buf, "YOU CAME IN 2ND PLACE");
		DrawCenteredText(buf, y);
		y += 90;
		if (this->gamewon == 0) {
			sprintf(buf, "YOU GET THE 2ND PLACE PRIZE OF ");
		} else {
			sprintf(buf, "AFTER A %d-DAY RUN AS JEOPARDY! CHAMPION", this->gamewon);
			DrawCenteredText(buf, y);
			y += 90;
			strcpy(buf, "YOUR TOTAL WINNINGS ARE ");
		}
		break;
	case 3:
		strcpy(buf, "YOU CAME IN 3RD PLACE");
		DrawCenteredText(buf, y);
		y += 90;
		if (this->gamewon == 0) {
			sprintf(buf, "YOU GET THE 3RD PLACE PRIZE OF ");
		} else {
			sprintf(buf, "AFTER A %d-DAY RUN AS CHAMPION", this->gamewon);
			DrawCenteredText(buf, y);
			y += 90;
			strcpy(buf, "YOUR TOTAL WINNINGS ARE ");
		}
		break;
	}

	strcpy(winstr, buf);
	strcpy(buf, DollarStr(am));
	strcat(winstr, buf);

	DrawCenteredText(winstr, y);
	y += 90;

	strcpy(buf, "PRESS A KEY TO CONTINUE");
	DrawCenteredText(buf, y);
}



int tkpbonus(int round, int row) {
	if (round > 0)
		return(0);	// no adjustment for DJ! round

	switch (row) {
	case 0:
	case 1:
		return (0); // easiest clues aren't significantly easier in the first Jeopardy! round
	case 2:
		return (2);	// middle row about 2% easier in SJ versus DJ
	case 3:
		return (4); // about 4% easier
	case 4:
		return (5); // about 5% easier in the first Jeopardy! round
	}

	return(0);
}

int Jeopardy::CalculateContestantAverageCoryat(int iopp) {
	int total;
	int e;
	int f;
	int r;
	int c;
	int g;

	total = 0;

	for (g = 0; g < 5000; ++g) {
		for (r = 0; r < 2; ++r) {
			for (f = 0; f < 6; ++f) {
				for (e = 200, c = 0; e <= 1000; e += 200, ++c) {
					if (RandPercent(this->tkp[iopp][c] + tkpbonus(r, c))) {
						// a "ring in"
						if (RandPercent(this->akp[iopp][c])) {
							if (r == 0)
								total += e;
							else
								total += e * 2;
						} else {
							if (r == 0)
								total -= e;
							else
								total -= e * 2;
						}
					}
				}
			}
		}
	}

	// return the average Coryat score
	return(total / g);
}

int Jeopardy::CalculateAverageCombinedCoryat(int iopp) {
	int total;
	int e;
	int f;
	int r;
	int c;
	int g;
	int o;

	total = 0;
#ifdef RINGIN_DEPENDENT
	for (g = 0; g < 5000; ++g) {
		for (r = 0; r < 2; ++r) {
			for (f = 0; f < 6; ++f) {
				for (e = 200, c = 0; e <= 1000; e += 200, ++c) {
					// each opponent
					for (o = 0; o < 3; ++o) {
						if (o > 0 && randbetween(0, 99) >= dep100)
							continue;		// player 1 didn't know, have to check against the dependent probability
						if (RandPercent(this->tkp[iopp][c] + tkpbonus(r, c))) {
							// a "ring in"
							if (RandPercent(this->akp[iopp][c])) {
								if (r == 0)
									total += e;
								else
									total += e * 2;
								// it's correct, no more ring-ins
								break;
							} else {
								if (r == 0)
									total -= e;
								else
									total -= e * 2;
							}
						}
					}
				}
			}
		}
	}
#else
	for (g = 0; g < 5000; ++g) {
		for (r = 0; r < 2; ++r) {
			for (f = 0; f < 6; ++f) {
				for (e = 200, c = 0; e <= 1000; e += 200, ++c) {
					// each opponent
					for (o = 0; o < 3; ++o) {
						if (o > 0 && randbetween(0, 99) >= dep100)
							continue;		// player 1 didn't know, have to check against the dependent probability
						if (RandPercent(this->tkp[iopp][c] + tkpbonus(r, c))) {
							// a "ring in"
							if (RandPercent(this->akp[iopp][c])) {
								if (r == 0)
									total += e;
								else
									total += e * 2;
								// it's correct, no more ring-ins
								break;
							} else {
								if (r == 0)
									total -= e;
								else
									total -= e * 2;
							}
						}
					}
				}
			}
		}
	}
#endif

	// return the average Coryat score
	return(total / g);
}


void Jeopardy::DrawSpecialty(void) {
	char buf[256];
	SBitmap *txt;
	int x;
	int y;

	y = 90;
	screen->clear(BOARD_CLR);

	strcpy(buf, "PLEASE TYPE YOUR SPECIALTY CATEGORY");
	DrawCenteredText(buf, y);
	this->DrawTypeBuffer();
}

void Jeopardy::DrawCenteredText(const char* str, int y) {
	SBitmap *txt;
	int x;
	txt = this->category_font->RenderWord(str);
	x = (this->w_x - txt->width()) / 2;
	if (txt->width() >= this->w_x) {
		txt->blit(txt->width() - this->w_x, 0, txt->width() - 1, txt->height() - 1, screen, 0, y);
	} else {
		txt->blit(screen, 0, 0, x, y, txt->width(), txt->height());
	}
	delete txt;
}

void Jeopardy::DrawContestants(void) {
	char buf[256];
	SBitmap *txt;
	int x;
	int y;
	int am;
	int i;

	y = 90;
	screen->clear(BOARD_CLR);

	for (i = 0; i < 2; ++i) {
		am = this->acoryat[i];

		sprintf(buf, "%s, %s", playernames[this->playerid[1 + i]], occupations[this->ioccup[i]]);
		DrawCenteredText(buf, y);
		y += 90;

		sprintf(buf, "FROM %s", wherefrom[this->ifrom[i]]);
		DrawCenteredText(buf, y);
		y += 90;
			
		sprintf(buf, "AVG. CORYAT SCORE %d", am);
		DrawCenteredText(buf, y);
		y += 90;
	}

	strcpy(buf, "PRESS A KEY TO CONTINUE");
	DrawCenteredText(buf, y);
}


void Jeopardy::Draw() {
	int e;

	if (!this->screen)
		return;

	switch (this->current_game_mode) {
	case mode_main_menu:
		this->DrawMainMenu();
		break;
	case mode_instructions:
		this->DrawInstructions();
		break;
	case mode_buzzer_params:
		this->DrawBuzzerDelayParam();
		break;
	case mode_buzzer_show_results:
		this->DrawBuzzerResults();
		break;
	case mode_board_reveal:
	case mode_board:
		this->introsound = false;
		this->wager = -1;
		this->ddsel = false;
		if (this->current_game_mode == mode_board && !this->boardsound) {
			this->boardsound = true;
			soundfx.play_sfx(this->sound_fx[sfx_boardstart1 + randbetween(0, 2)]);
		}
		this->DrawBoard();
		break;
	case mode_category_reveal:
		this->boardsound = false;
		this->boardsound2 = false;
		if (this->clue_bmp != NULL)
			this->clue_bmp->blit(this->screen, 0, 0, 0, 0, this->clue_bmp->width(), this->clue_bmp->height());
		if (!introsound) {
			switch (this->round)
				{
			case 0:
				soundfx.play_sfx(this->sound_fx[sfx_round1intro + randbetween(0, 3)]);
				break;
			case 1:
				soundfx.play_sfx(this->sound_fx[sfx_round2intro + randbetween(0, 3)]);
				break;
			case 2:
				soundfx.play_sfx(this->sound_fx[sfx_round3intro]);
				break;
				}
			this->introsound = true;
		}
		break;
	case mode_animating_clue:
		this->AnimateClue();
		break;
	case mode_clue:
	case mode_buzzer_clue:
	case mode_aiplayer_answers:
		if (this->current_game_mode == mode_buzzer_clue) {
			if (this->clue_bmp)
				this->clue_bmp->stretch_blit(this->screen, 0, 0, clue_bmp->width(), clue_bmp->height(), 0, 0, screen->width(), screen->height());
			if (this->lightson && ticks < this->lighttime) {
				/* show the "go lights" */ 
				const int nlights = 12;
				const int lightwidth = 10;
				const int lightheight = 16;
				const int lightcolor = 0xffffaf;

				for (e = 0; e < nlights; ++e) {
					screen->rect_fill(0, (e + 1) * this->screen->height() / (nlights + 1) - lightheight, lightwidth, (e + 1) * this->screen->height() / (nlights + 1) + lightheight, lightcolor);
					screen->rect_fill(this->screen->width() - 1 - lightwidth, (e + 1) * this->screen->height() / (nlights + 1) - lightheight, this->screen->width() - 1, (e + 1) * this->screen->height() / (nlights + 1) + lightheight, lightcolor);
				}
			}
			if (this->showtime) {
				char buf[256];
				SBitmap *txt;
				float hs;
				hs = float(this->hsec) * 0.01;
				if (this->buzzerscore < 0) {
					sprintf(buf, "TIME: %0.2f s", hs);
					DrawCenteredText(buf, 10);
				}
			}
		} else if ((this->showanswer && this->round < 2) || (this->rdown && this->round != 2 && !this->isgame)) { // do not reveal question in FJ!
			ques_bmp->stretch_blit(this->screen, 0, 0, ques_bmp->width(), ques_bmp->height(), 0, 0, screen->width(), screen->height());
		} else {
			clue_bmp->stretch_blit(this->screen, 0, 0, clue_bmp->width(), clue_bmp->height(), 0, 0, screen->width(), screen->height());
		}

		if (this->isgame) {
			if (this->showamount)
				this->DrawScore();
			else if (current_game_mode == mode_aiplayer_answers)
				this->DrawAITypeBuffer();
			else
				this->DrawTypeBuffer();
		}
		if (this->game_type == GAME_QUIZ)
			this->DrawExamStatus();
		if (current_game_mode == mode_aiplayer_answers && !currently_generating() && !answerchecked) {
			this->CheckAIAnswer(false);
		}
		break;
	case mode_wager:
		this->DrawWager();
		break;
	case mode_fj3rd:
		this->DrawFJReveal(2);
		break;
	case mode_fj2nd:
		this->DrawFJReveal(1);
		break;
	case mode_fj1st:
		this->DrawFJReveal(0);
		break;
	case mode_opponent_dd_wager:
		this->DrawDDWager();
		break;
	case mode_opponent_dd_answer:
		clue_bmp->stretch_blit(this->screen, 0, 0, clue_bmp->width(), clue_bmp->height(), 0, 0, screen->width(), screen->height());
		break;
	case mode_aiopponent_dd_answer:
		clue_bmp->stretch_blit(this->screen, 0, 0, clue_bmp->width(), clue_bmp->height(), 0, 0, screen->width(), screen->height());
		this->DrawAITypeBuffer();
		if (!currently_generating() && !answerchecked) {
			this->CheckAIAnswer(true);
		}
		break;
	case mode_opponent_dd_question:
		ques_bmp->stretch_blit(this->screen, 0, 0, clue_bmp->width(), clue_bmp->height(), 0, 0, screen->width(), screen->height());
		this->DrawScore();
		break;
	case mode_game_end:
		this->DrawGameEnd();
		break;
	case mode_introduce_contestants:
		this->DrawContestants();
		break;
	case mode_specialtyselect:
		this->DrawSpecialty();
		break;
	}
}

void Jeopardy::FillDateTime(int &date, int &tm) {
	time_t tim;
	struct tm *ptm;

	time(&tim);
	ptm = localtime(&tim);

	date = (ptm->tm_year + 1900) * 10000;
	date += (ptm->tm_mon + 1) * 100;
	date += ptm->tm_mday;

	tm = ptm->tm_hour * 10000 + ptm->tm_min * 100 + ptm->tm_sec;
}

void Jeopardy::InitGameStats(void) {
	int e;
	int f;

	if (this->game_type == GAME_JEOPARDY3)
		gamestats.type = 1;
	else if (this->game_type == GAME_JEOPARDY)
		gamestats.type = 0;
	else
		gamestats.type = 2;

	FillDateTime(gamestats.date, gamestats.time);

	gamestats.qright = 0;
	gamestats.qwrong = 0;
	gamestats.ddright = 0;
	gamestats.ddwrong = 0;
	gamestats.valright = 0;
	gamestats.valwrong = 0;
	gamestats.coryat = 0;
	gamestats.fjright = 0;
	gamestats.score = 0;
	gamestats.scoreopp1 = 0;
	gamestats.scoreopp2 = 0;
	gamestats.gameseries++;
	gamestats.coryatdd = 0;

	for (e = 0; e < 2; ++e) {
		for (f = 0; f < 5; ++f) {
			gamestats.rqright[e][f] = 0;
			gamestats.rqwrong[e][f] = 0;
			gamestats.rddright[e][f] = 0;
			gamestats.rddwrong[e][f] = 0;
		}
	}
}

void Jeopardy::BoardControl(void) {
	int e;
	int his;

	if (this->control == 0) {
		clback = NULL;
	} else {
		bool ddstrat;

		his = this->playerscore;
		if (this->oppscore[0] > his)
			his = this->oppscore[0];
		if (this->oppscore[1] > his)
			his = this->oppscore[1];

		ddstrat = false;
		if (this->round == 1 && 
			(this->oppscore[this->control - 1] == his || this->oppscore[this->control - 1] * 2 >= his)
			&& RandPercent(50))
			ddstrat = true;

		if (ddstrat) {
			// Daily Double hunting strategy
			s_a = 4;
			while (s_a > 0)
				{
				for (e = 0; e < 6; ++e)
					{
					// avoid categories that already have the DD revealed
					if (this->dd_c[0] == e && this->selected[this->dd_c[0]][this->dd_a[0]])
						continue;
					if (this->dd_c[1] == e && this->selected[this->dd_c[1]][this->dd_a[1]])
						continue;
					// else, try this one
					if (!this->selected[e][4])
						break;
					}
				if (e < 6)
					{
					s_c = e;
					goto SFound; 
					}
				--s_a;
				}
			goto RCat;
		} else if (RandPercent(70)) {
			// same category
			if (this->goup) {
				for (e = 4; e >= 0; --e)
					if (!this->selected[lastcatsel][e])
						break;
			} else {
				for (e = 0; e < 5; ++e)
					if (!this->selected[lastcatsel][e])
						break;
			}
			if (e < 5 && e >= 0) {
				s_c = lastcatsel;
				s_a = e;
			} else {
				goto RCat;
			}
		} else {
			// new category
RCat:
			if (RandPercent(10))
				this->goup = true;
			else
				this->goup = false;

			forever {
				lastcatsel = randbetween(0, 5);
				if (this->goup) {
					for (e = 4; e >= 0; --e)
						if (!this->selected[lastcatsel][e])
							break;
				} else {
					for (e = 0; e < 5; ++e)
						if (!this->selected[lastcatsel][e])
							break;
				}
				if (e < 5 && e >= 0) {
					s_c = lastcatsel;
					s_a = e;
					break;
				}
			}
		}

SFound:
		clback = this;
		this->tick_start = ticks;
		this->ForceRedraw();
	}
}

void Jeopardy::DoReveal(void) {
	/* animates the board reveal */
	int c;
	int e;
	int f;

	if (ticks - this->tick_start >= 28) {
		// reveal two more dollar amounts
		c = 0;
		for (e = 0; e < 6; ++e) {
			for (f = 0; f < 5; ++f) {
				if (!this->revealed[e][f])
					c++;
			}
		}
		if (c == 0) {
			clback = NULL;
			this->current_game_mode = mode_board;
			this->BoardControl();
			this->ForceRedraw();
			return;
		}
		forever {
			e = randbetween(0, 14);
			f = e / 6;
			e -= f * 6;
			if (!this->revealed[e][f]) {
				this->revealed[e][f] = true;
				break;
			}
		}
		forever {
			e = 15 + randbetween(0, 14);
			f = e / 6;
			e -= f * 6;
			if (!this->revealed[e][f]) {
				this->revealed[e][f] = true;
				break;
			}
		}

		// and show the new amounts
		this->ForceRedraw();
		this->tick_start = ticks;
	}
}

void Jeopardy::DrawFJReveal(int place) {
	char buf[256];
	char add[32];
	const char *plcs[] = {"1ST", "2ND", "3RD"};
	SBitmap *txt;
	int x;
	int y;
	int e;
	int am;

	this->screen->clear(BOARD_CLR);

	sprintf(buf, "IN %s PLACE, %s", plcs[place], playernames[this->playerid[plc[place]]]);
	y = 80;
	DrawCenteredText(buf, y);

	strcpy(buf, "WITH ");
	am = this->fjs[place];
	strcat(buf, DollarStr(am));
	strcat(buf, " GOING INTO FINAL JEOPARDY!");

	y += 90;
	DrawCenteredText(buf, y);

	strcpy(buf, "WAGER IS: ");
	if (plc[place] == 0)
		am = this->wager;
	else
		am = this->fjwager[plc[place] - 1];
	strcat(buf, DollarStr(am));

	y += 90;
	DrawCenteredText(buf, y);

	if (llama == nullptr || plc[place] == 0) {
		if (plc[place] == 0) {
			if (_adjsc)
				this->CheckAnswer();
			if (this->lastqright) {
				strcpy(buf, "YOUR FINAL JEOPARDY! QUESTION IS CORRECT");
				am = this->fjs[plc[place]] + this->wager;
			} else {
				strcpy(buf, "FINAL JEOPARDY! QUESTION IS INCORRECT");
				am = this->fjs[plc[place]] - this->wager;
				judgedock = this->wager;

			}
		} else {
			if (this->fjright[plc[place] - 1]) {
				strcpy(buf, "YOUR FINAL JEOPARDY! QUESTION IS CORRECT");
				am = this->fjs[plc[place]] + this->fjwager[plc[place] - 1];
				if (_adjsc)
					this->oppscore[plc[place] - 1] += this->fjwager[plc[place] - 1];
			} else {
				strcpy(buf, "FINAL JEOPARDY! QUESTION IS INCORRECT");
				am = this->fjs[plc[place]] - this->fjwager[plc[place] - 1];
				if (_adjsc)
					this->oppscore[plc[place] - 1] -= this->fjwager[plc[place] - 1];
			}
		}

		y += 90;
		DrawCenteredText(buf, y);

		strcpy(buf, "DEPRESS A KEY TO CONTINUE");
		y += 90;
		DrawCenteredText(buf, y);

		this->DrawScore();

		_adjsc = false;
	} else {
		if (!fjgen[plc[place] - 1]) {
			// begin generation
			fjgen[plc[place] - 1] = true;
			request_generation(llama, final_cat, final_clue);
		} else if (!currently_generating()) {
			if (_adjsc)
				CheckAIAnswer(false);
			if (lastqright) {
				strcpy(buf, "YOUR FINAL JEOPARDY! QUESTION IS CORRECT");
				if (_adjsc)
					this->oppscore[plc[place] - 1] += this->fjwager[plc[place] - 1];
			} else {
				strcpy(buf, "FINAL JEOPARDY! QUESTION IS INCORRECT");
				if (_adjsc)
					this->oppscore[plc[place] - 1] -= this->fjwager[plc[place] - 1];
			}

			y += 90;
			DrawCenteredText(buf, y);

			strcpy(buf, "DEPRESS A KEY TO CONTINUE");
			y += 90;
			DrawCenteredText(buf, y);

			this->DrawScore();

			_adjsc = false;
		} else {
			DrawAITypeBuffer();
		}
	}

}

void Jeopardy::Size(int cx, int cy) {
	this->w_x = cx;
	this->w_y = cy;

	if (!this->fGfxInit)
		return;

	if (screen != nullptr)
		delete screen;
	screen = new SBitmap(cx, cy);

	this->catdrawn = false;
	this->DDBitmap();
}

void Jeopardy::MouseMove(int x, int y) {
	if (x < 0 || y < 0 || x >= APP_WIDTH || y >= APP_HEIGHT)
		return;
	// record mouse position
	if (this->current_game_mode == mode_board || this->current_game_mode == mode_main_menu) {
		if (this->current_game_mode == mode_board && this->control > 0) {
			m_x = 0;
			m_y = 0;
		} else {
			m_x = x;
			m_y = y;
		}
		this->ForceRedraw();
	}
	if (this->ddsel)
		this->ForceRedraw();
}

void Jeopardy::FetchNextClue(bool ts) {
	int catid;
	char *cmd;
	int err;
	int clueid;

	forever {
		strret[0] = 0;
		catid = randbetween(0, this->max_catid);
		clueid = randbetween(0, 5);

		_lookfor = clueid;
		cmd = sqlite3_mprintf("SELECT answer FROM Clue WHERE catid=%d;", catid);
		err = sqlite3_exec(this->db, cmd, clue_callback, NULL, NULL);
		sqlite3_free(cmd);

		if (strret[0] == 0)
			continue;

		this->final_clue = new char [strlen(strret) + 1];
		strcpy(this->final_clue, strret);

		_lookfor = clueid;
		cmd = sqlite3_mprintf("SELECT question FROM Clue WHERE catid=%d;", catid);
		err = sqlite3_exec(this->db, cmd, clue_callback, NULL, NULL);
		sqlite3_free(cmd);

		this->final_question = new char [strlen(strret) + 1];
		strcpy(this->final_question, strret);

		cmd = sqlite3_mprintf(
			"SELECT Name FROM Category WHERE id=%d;",
			catid);
		err = sqlite3_exec(this->db, cmd, str_callback, NULL, NULL);
		sqlite3_free(cmd);

		this->final_cat = new char [strlen(strret) + 1];
		strcpy(this->final_cat, strret);

		break;
	}
}


void Jeopardy::DoCategoryReveal(void) {
	if ((this->catreveal < 0 && ticks - this->tick_start > 50) || ticks - this->tick_start > 250) {
		SBitmap *catbmp;
		int nw, ys, yp;
		char *cmd;
		int err;

		if (this->game_type == GAME_QUIZ) {
			if (this->catreveal >= 0) {
				clback = NULL;
				this->catreveal = -1;
				this->typebuffer[0] = 0;
				this->tick_start = ticks;
				this->PrepareToAcceptInput();
				this->CreateClueBitmap(this->final_clue, false);
				this->current_game_mode = mode_clue;
				this->ForceRedraw(true);
				return;
			}
			if (this->nquestions == 50) {
				// game over
				this->current_game_mode = mode_main_menu;
				clback = NULL;
				this->ForceRedraw(true);
				return;
			}
			this->nquestions++;
			this->catreveal++;
			this->FetchNextClue(false);

			strcpy(strret, this->final_cat);

			if (this->clue_bmp != NULL) {
				delete clue_bmp;
				this->clue_bmp = nullptr;
			}
			this->clue_bmp = new SBitmap(this->w_x, this->w_y);
			clue_bmp->clear(BOARD_CLR);
		} else if (this->game_type == GAME_CHALLENGE)	{
		} else {
			this->catreveal++;
			if (this->catreveal > 0 && this->round == 2) {
				this->current_game_mode = mode_wager;
				this->PrepareToAcceptInput();
				this->ForceRedraw();
				return;
			}
			if (this->catreveal > 5) {
				this->current_game_mode = mode_board_reveal;
				this->ForceRedraw();
				return;
			}
			if (this->clue_bmp != NULL) {
				delete clue_bmp;
				this->clue_bmp = nullptr;
			}
			this->clue_bmp = new SBitmap(this->w_x, this->w_y);
			clue_bmp->clear(BOARD_CLR);

			if (this->round == 2) {
				// Show the Final Jeopardy! category
				strcpy(strret, this->final_cat);
			} else {
				cmd = sqlite3_mprintf(
					"SELECT Name FROM Category WHERE id=%d;",
					this->cats[this->catreveal + this->round * 6]);
				err = sqlite3_exec(this->db, cmd, str_callback, NULL, NULL);
				sqlite3_free(cmd);
			}
		}

#if 1
		catbmp = this->category_font->RenderCategory(strret);

		if (strlen(strret) < 8)
			nw = (this->w_x / 2) * 9 / 10;
		else
			nw = this->w_x * 9 / 10;
		ys = catbmp->height() * nw / catbmp->width();
		while (ys > this->w_y) {
			ys *= 4;
			ys /= 5;
		}
		yp = this->w_y / 2 - ys / 2;

		catbmp->stretch_blit(this->clue_bmp, 0, 0, catbmp->width(), catbmp->height(), (this->w_x - nw) / 2, yp, nw, ys);
		delete catbmp;
#else
		catbmp = __tr.RenderCategory(strret);

		if (strlen(strret) < 8)
			nw = (this->w_x / 2) * 9 / 10;
		else
			nw = this->w_x * 9 / 10;
		ys = catbmp->height() * nw / catbmp->width();
		while (ys > this->w_y)
			{
			ys *= 4;
			ys /= 5;
			}
		yp = this->w_y / 2 - ys / 2;

		stretch_blit(catbmp, this->clue_bmp, 0, 0, catbmp->width(), catbmp->height(), (this->w_x - nw) / 2, yp, nw, ys);
		destroy_bitmap(catbmp);
#endif

		this->ForceRedraw();
		this->tick_start = ticks;
	}
}


void Jeopardy::FJHandler(void) {
	static int fjticks = 3500;

	if (!this->thinkmusic && ticks - tick_start > 500) {
		// start the music
		int usesfx;

		this->thinkmusic = true;
		clback = this;
		this->tick_start = ticks;

		usesfx = randbetween(0, 2);
		switch (usesfx) {
		case 0:
		default:
			fjticks = 3500;
			break;
		case 1:
		case 2:
			fjticks = 3300;
			break;
		}

		soundfx.play_sfx(this->sound_fx[sfx_think + usesfx]);
	} else if (ticks - tick_start > fjticks) {
		// show the big answer (question, natch)
		if (this->game_type == GAME_JEOPARDY3) {
			this->current_game_mode = mode_fj3rd;
			_adjsc = true;
			this->ForceRedraw();
		} else {
			this->tick_start = ticks;
			this->CreateClueBitmap(this->final_question, false);
			this->showamount = true;
			this->CheckAnswer();
			this->fjdone = true;
		}
	}
}


bool _timeout = false;
void Jeopardy::TimerOut(void) {
	int timering;
	int timeanswer;
	static int insidehere = false;

	if (insidehere)
		return;
	insidehere = true;

#ifdef	CODEHAPPY_WASM
	timering = 1200;
	timeanswer = 1500;
#else	
	timering = 600;
	timeanswer = 1000;
#endif	

	if (this->ddsel) {
		// extra time for Daily Doubles
		timering *= 3;
		timeanswer *= 3;
	}

	if ((!inputstart && !this->ddsel && (ticks - this->tick_start >= timering)) ||
		(inputstart && ticks - this->tick_start >= timeanswer)) {
		if (!this->timesound) {
			int ip;
			this->canring[0] = false;
			if (this->game_type == GAME_JEOPARDY3 && !this->ddsel && !this->answerchecked && (canring[1] || canring[2])) {
				// give opponents a chance to ring in
				bool inputstsav;

				inputstsav = this->inputstart;
				ip = WhoRings(false);
				if (ip > 0) {
					_timeout = true;
					this->OpponentRingin(ip);
					_timeout = false;
					this->ForceRedraw();
					if (llama == nullptr) {
						this->showamount = true;
						this->inputstart = inputstsav;
						goto DoneClue;
					} else {
						this->inputstart = inputstsav;
						insidehere = false;
						return;
					}
				}
			}
			this->timesound = true;
			soundfx.play_sfx(this->sound_fx[sfx_time]);
			response.ringin = 0;
			response.right = 0;
			response.catid = this->cats[this->s_c + this->round * 6];
			response.value = (this->s_a + 1) * 200;
			if (this->round > 0)
				response.value *= 2;
			if (this->round == 2)
				response.value = -1;
			response.round = this->round;
			response.added = false;
DoneClue:
			clback = nullptr;
			this->showanswer = true;
			this->drawtimer = false;
			TurnBuzzersOff();
			if (this->game_type == GAME_QUIZ) {
				// continue to the next question
				this->drawtimer = false;
				this->CheckAnswer();
				this->answerchecked = true;
			} else if (this->ddsel || this->inputstart) {
				// unfortunately you're in jeopardy during a D.D., whether you answer or not
				if (this->ddsel) {
					this->playerscore -= this->wager;
				} else {
					int damount;
					damount = (this->s_a + 1) * 200;
					if (this->round == 1)
						damount += damount;	// Double Jeopardy!
					this->playerscore -= damount;
				}

				this->showamount = true;
			}

			this->ForceRedraw();
		}
	}

	if (this->drawtimer && inputstart && !this->timesound) {
		this->timerlen = TIMERLENGTH - (TIMERLENGTH * (ticks - this->tick_start) / timeanswer);
		if (this->timerlen < 0)
			this->timerlen = 0;
		if ((this->tcount & 7) == 0)
			this->ForceRedraw();
		this->tcount++;
	}

	insidehere = false;
}

void Jeopardy::TimerCallback(void) {
	switch (this->current_game_mode) {
	case mode_animating_clue:
		// force a redraw to draw the animated fly-out clue
		this->ForceRedraw();
		break;
	case mode_board_reveal:
		this->DoReveal();
		break;
	case mode_category_reveal:
		this->DoCategoryReveal();
		break;
	case mode_aiplayer_answers:
		// don't null the timer callback
		break;
	case mode_clue:
		if (this->round == 2) {
			this->FJHandler();
		} else {
			this->TimerOut();
		}
		break;
	case mode_board:
		if (ticks - this->tick_start > 200) {
			this->SelectClue();
			this->ForceRedraw();
		}
		break;
	case mode_buzzer_clue:
		if (this->tickcall >= 0 && ticks >= this->tickcall && this->namecall >= 0) {
			this->tickcall = -1;
			soundfx.play_sfx(this->sound_fx[sfx_playerchris + playerid[0 + this->namecall]]);
			this->voicecall = 0;
			this->calldonetick = ticks + 50; //this->sound_fx[sfx_playerchris + playerid[0 + this->namecall]]->len / this->sound_fx[sfx_playerchris + playerid[0 + this->namecall]]->freq;
		}
		if (this->ndone > 0 && ticks - this->tick_start > this->readinglen + 300) {
			if (this->buzzerscore < 0 || this->voicecall < 0 || ticks > this->calldonetick) {
				this->voicecall = -1;
				this->BuzzerClue();
				this->ForceRedraw(false);
			}
		}
#if 0
		if (!this->lightson && this->voiced < 0) {
			if (ticks - this->tick_start > 100) {
				if (this->mdelay > 0) {
					if (voice_get_position(this->voicen) < 0) {
						if (this->ndone < BUZZERSIMCLUES)
							this->voiced = ticks;
					}
				} else {
					int vpos;

					vpos = voice_get_position(this->voicen);
					vpos = this->voicet - vpos;
					vpos = (vpos * 100) / this->voicef;
					vpos = -vpos;

					this->voiced = ticks - vpos;
				}
			}
		}
#endif
		if (this->lightson && ticks > this->lighttime && this->lighttime > 0) {
			this->ForceRedraw();
			this->lighttime = 0;
		}
		if (!this->lightson && this->voiced > 0 && ticks - this->voiced >= this->mdelay) {
			this->lightson = true;
			this->lighttime = ticks + 10 + randbetween(0, 19);
			this->ForceRedraw();
		}
		break;
	default:
		clback = nullptr;
		break;
		}
}

void Jeopardy::CreateClueBitmap(char *str, bool question) {
	SBitmap *cb, * cbs;
	int nw, nh;

	cb = this->answer_font->RenderCategory(str);

	if (question)
		strcpy(this->accept, str);

	if (!question) {
		nw = this->w_x * 5 / 6;
		nh = cb->height() * nw / cb->width();

		if (nh > this->w_y) {
			nh = this->w_y * 5 / 6;
			nw = cb->width() * nh / cb->height();
		}
		cur_clue_text = str;
	} else {
		nw = cb->width();
		nh = cb->height();
		nw += nw;
		nh += nh;
		if (nw > this->w_x || nh > this->w_y) {
			nw = this->w_x * 5 / 6;
			nh = cb->height() * nw / cb->width();

			if (nh > this->w_y) {
				nh = this->w_y * 5 / 6;
				nw = cb->width() * nh / cb->height();
			}
		}
	}

	cbs = new SBitmap(nw, nh);
	cb->stretch_blit(cbs, 0, 0, cb->width(), cb->height(), 0, 0, cbs->width(), cbs->height());

	if (!question) {
		clue_bmp = new SBitmap(w_x, w_y);
		clue_bmp->clear(BOARD_CLR);
		cbs->blit(this->clue_bmp, 0, 0, (this->w_x - cbs->width()) / 2, (this->w_y - cbs->height()) / 2, clue_bmp->width(), clue_bmp->height());
	} else {
		ques_bmp = new SBitmap(w_x, w_y);
		ques_bmp->clear(BOARD_CLR);
		cbs->blit(this->ques_bmp, 0, 0, (this->w_x - cbs->width()) / 2, (this->w_y - cbs->height()) / 2, ques_bmp->width(), ques_bmp->height());
	}

	this->cx1 = this->s_c * this->w_x / 6;
	this->cx2 = (this->s_c + 1) * this->w_x / 6;
	this->cy1 = (this->s_a + 1) * this->w_y / 6;
	this->cy2 = (this->s_a + 2) * this->w_y / 6;

	delete cb;
	delete cbs;
}


void Jeopardy::ClueBitmap(bool question) {
	char *cmd;
	int err;

	if (!question && this->clue_bmp != nullptr) {
		delete clue_bmp;
		clue_bmp = nullptr;
	}
	if (question && this->ques_bmp != nullptr) {
		delete ques_bmp;
		ques_bmp = nullptr;
	}

	_lookfor = this->s_a;
	if (!question) {
		cmd = sqlite3_mprintf(
			"SELECT answer FROM Clue WHERE catid=%d;",
			this->cats[this->s_c + this->round * 6]);
	} else {
		cmd = sqlite3_mprintf(
			"SELECT question FROM Clue WHERE catid=%d;",
			this->cats[this->s_c + this->round * 6]);
	}
	err = sqlite3_exec(this->db, cmd, clue_callback, NULL, NULL);
	sqlite3_free(cmd);

	this->CreateClueBitmap(strret, question);
}


void Jeopardy::SaveGame(void) {
	char *cmd;
	int err;
	int e;
	int f;

	if (this->game_hard || this->game_specialty)
		return;	// don't save results for "hard" mode games

	switch (this->game_type) {
	case GAME_JEOPARDY:
		cmd = sqlite3_mprintf(
			"INSERT INTO SoloGame VALUES (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d);",
			gamestats.date, gamestats.time,
			gamestats.qright, gamestats.qwrong,
			gamestats.ddright, gamestats.ddwrong,
			gamestats.valright, gamestats.valwrong,
			gamestats.coryat, gamestats.fjright,
			this->playerscore, gamestats.coryatdd);
		err = sqlite3_exec(this->db, cmd, NULL, NULL, NULL);
		sqlite3_free(cmd);
		this->SaveCatmap();
		for (e = 0; e < 2; ++e) {
			for (f = 0; f < 5; ++f) {
				cmd = sqlite3_mprintf(
					"INSERT INTO RowPerf VALUES (%d,%d,%d,%d,%d,%d,%d,%d);",
					gamestats.date, gamestats.time,
					e, f,
					gamestats.rqright[e][f], gamestats.rqwrong[e][f],
					gamestats.rddright[e][f], gamestats.rddwrong[e][f]
					);
				err = sqlite3_exec(this->db, cmd, NULL, NULL, NULL);
				sqlite3_free(cmd);
			}
		}
		break;
	case GAME_JEOPARDY3:
		cmd = sqlite3_mprintf(
			"INSERT INTO OppGame VALUES (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d);",
			gamestats.date, gamestats.time,
			gamestats.qright, gamestats.qwrong,
			gamestats.ddright, gamestats.ddwrong,
			gamestats.valright, gamestats.valwrong,
			gamestats.coryat, gamestats.fjright,
			this->playerscore,
			this->oppscore[0], this->oppscore[1], gamestats.gameseries,
			gamestats.coryatdd);
		err = sqlite3_exec(this->db, cmd, NULL, NULL, NULL);
		sqlite3_free(cmd);
		this->SaveCatmap();
		break;
	}
}

bool Jeopardy::CheckRound(void) {
	/* returns true if we've advanced a round */
	int e;
	int f;

	if ((response.ringin > 0 || this->game_type == GAME_JEOPARDY) && !response.added) {
		// add clue info
		char *cmd;
		int err;

		response.added = true;

		if (response.ringin == 0)
			response.response[0] = 0;

		cmd = sqlite3_mprintf(
			"INSERT INTO QResponse VALUES (%d,%d,%d,%d,%d,%d,%d,'%q',%d);",
			response.type, response.date, response.time,
			response.catid, response.value, response.round,
			response.ringin, response.response, response.right);
		err = sqlite3_exec(this->db, cmd, NULL, NULL, NULL);
		sqlite3_free(cmd);

		if (response.ringin != 0) {
			if (response.right > 0) {
				gamestats.qright++;
				if (this->round < 2 && (this->ddsel || response.ringin == 2)) {
					gamestats.ddright++;
					gamestats.rddright[this->round][this->s_a]++;
				}
				if (this->round < 2) {
					gamestats.coryat += response.value;
					gamestats.coryatdd += response.value;
					gamestats.valright += response.value;
					gamestats.rqright[this->round][this->s_a]++;
				}
				if (this->round == 2)
					gamestats.fjright = 1;
			} else {
				gamestats.qwrong++;
				if (this->round < 2 && (this->ddsel || response.ringin == 2)) {
					gamestats.ddwrong++;
					gamestats.rddwrong[this->round][this->s_a]++;
				}
				if (this->round < 2) {
					if (this->ddsel || response.ringin == 2)
						;	// don't adjust the DD-Coryat -- we're forced to answer on this one
					else
						gamestats.coryatdd -= response.value;
					
					gamestats.coryat -= response.value;
					gamestats.valwrong -= response.value;
					gamestats.rqwrong[this->round][this->s_a]++;
				}
				if (this->round == 2)
					gamestats.fjright = 0;
			}
		}

		response.ringin = 0;
	}

	if (this->round == 2) {
		if (!this->fjdone)
			return(false);		// not yet
		else {
			// start a new game
			this->SaveGame();
			this->round = 0;
			this->judgedock = -1;
			this->introsound = false;
			if (this->game_type == GAME_JEOPARDY) {
				this->RoundBegin();
			} else {
				this->CreateContestants();
				if (this->game_type == GAME_JEOPARDY3 && this->place == 1)
					this->PlayIntroSwoosh();
				this->current_game_mode = mode_introduce_contestants;
			}
			return(true);
		}
	}

	for (e = 0; e < 6; ++e) {
		for (f = 0; f < 5; ++f)
			if (!this->selected[e][f])
				break;
		if (f < 5)
			break;
	}
	if (e < 6)
		return(false);

	this->round++;
	if (this->round == 1) {
		// Double Jeopardy!
	} else if (this->round == 2) {
		// advance to Final Jeopardy
		this->CreateClueBitmap(this->final_clue, false);
	}

	this->RoundBegin();

	return(true);
}


void Jeopardy::SelectClue(void) {
	static bool inside = false;

	if (inside)
		return;
	inside = true;

	this->tick_start = ticks;
	this->ClueBitmap(false);
	this->ClueBitmap(true);
	this->selected[this->s_c][this->s_a] = true;
	clback = this;
	lastcatsel = this->s_c;

	this->answerchecked = false;

	switch (this->control) {
	case 0:
		was_lead(wasleading, playerscore, oppscore[0], oppscore[1]);
		break;
	case 1:
		was_lead(wasleading, oppscore[0], playerscore, oppscore[1]);
		break;
	case 2:
		was_lead(wasleading, oppscore[1], playerscore, oppscore[0]);
		break;
	}

	this->judgedock = -1;

	this->ddsel = false;
	if (this->s_a == this->dd_a[0] && this->s_c == this->dd_c[0])
		this->ddsel = true;
	if (this->s_a == this->dd_a[1] && this->s_c == this->dd_c[1])
		this->ddsel = true;

	if (this->ddsel) {
		int sid;

		forever {
			sid = sfx_double1 + randbetween(0, 15);
			if (sid == sfx_double6 && this->round < 1)
				continue;
			if (sid == sfx_double11 && !this->ddseen)
				continue;
			if (sid == sfx_double12 && !this->ddseen)
				continue;
			if (sid == sfx_double16 && !this->ddseen)
				continue;
			if (sid == sfx_double13 && !this->wasleading)
				continue;
			if ((sid == sfx_double14 || sid == sfx_double15) && (this->wasleading ||
				this->playerscore * 2 < this->oppscore[0] || 
				this->playerscore * 2 < this->oppscore[1]))
				continue;
			break;
		}
		soundfx.play_sfx(this->sound_fx[sid]);
		this->ddseen = true;
	}

	this->InitClue3Player();

	if (false /* _texttospeechmode */) {
		// here's where we can insert VALL-E Alex
	}

	inside = false;
	this->current_game_mode = mode_animating_clue;
}


void Jeopardy::CheckWinnings(void) {
	int plc[3];
	int score[3];
	int e;
	int f;

	plc[0] = 0;
	plc[1] = 1;
	plc[2] = 2;
	score[0] = this->playerscore;
	score[1] = this->oppscore[0];
	score[2] = this->oppscore[1];

	for (e = 0; e < 3; ++e) {
StartAgain:
		for (f = e + 1; f < 3; ++f) {
			if (score[e] < score[f]) {
				int sw;
				sw = score[e];
				score[e] = score[f];
				score[f] = sw;
				sw = plc[e];
				plc[e] = plc[f];
				plc[f] = sw;
				goto StartAgain;
			}
		}
	}

	if (plc[0] == 0) {
		this->place = 1;
		this->winnings += this->playerscore;
		this->gamewon++;
	} else if (plc[1] == 0) {
		this->place = 2;
		this->winnings += 2000;
	} else if (plc[2] == 0) {
		this->place = 3;
		this->winnings += 1000;
	}
}

void Jeopardy::PlayIntroSwoosh(void) {
	// play the introduction sound, and let's go!
	// THIS... IS... JEOPARDY!
	soundfx.play_sfx(this->sound_fx[sfx_gameintro1 + randbetween(0, 2)]);
}


void Jeopardy::BuzzerClue(void) {
}


void Jeopardy::LButtonUp()
{
	// A selection has been made: transition modes
	switch (this->current_game_mode) {
	case mode_board:
		// valid clue selection?
		if (this->s_c > -1 && !this->selected[this->s_c][this->s_a]) {
			this->SelectClue();
		}
		break;

	case mode_game_end:
		break;

	case mode_clue:
		if (this->round < 2 || this->fjdone) {
			clback = nullptr;	// stop the "doo-doo-doo" timer, if it's running
			if (this->game_type == GAME_JEOPARDY3 && this->fjdone) {
				this->CheckWinnings();
				this->current_game_mode = mode_game_end;
			}
			else if (!this->CheckRound()) {
				this->current_game_mode = mode_board;
				this->BoardControl();
			}
			this->ForceRedraw(true); 
		} else if (this->isgame && this->current_game_mode == mode_clue && showanswer) {
			if (!this->CheckRound()) {
				this->current_game_mode = mode_board;
				this->BoardControl();
			}
			ForceRedraw(true);
		}
		break;

	case mode_buzzer_show_results:
		this->current_game_mode = mode_main_menu;
		this->ForceRedraw(true); 
		break;

	case mode_main_menu:
		// valid main menu selection?
		switch (this->mmsel) {
		case 0:
		case 6:
			this->current_game_mode = mode_category_reveal;
			this->game_type = GAME_JEOPARDY;
			if (this->mmsel == 0)
				this->game_hard = false;
			else
				this->game_hard = true;
			this->game_specialty = false;
			this->RoundBegin();
			this->ForceRedraw(true);
			break;
		case 1:
			this->game_specialty = false;
			this->CreateContestants();
			this->PlayIntroSwoosh();
			this->current_game_mode = mode_introduce_contestants;
			this->game_type = GAME_JEOPARDY3;
			gamestats.gameseries = -1;
			this->ForceRedraw(true);
			break;
		case 2:
			this->current_game_mode = mode_instructions;
			this->game_type = GAME_QUIZ;
			this->nquestions = 0;
			this->nright = 0;
			this->catreveal = -1;
			this->lastq = NULL;
			InitGameStats();
			this->ForceRedraw(true);
			break;
		case 3:
			this->current_game_mode = mode_instructions;
			this->game_type = GAME_CHALLENGE;
			this->ForceRedraw(true);
			break;
		case 4:
			// buzzer game #1
			this->current_game_mode = mode_instructions;
			this->game_type = GAME_BUZZER;
			this->buzzerscore = -1;
			this->ForceRedraw(true);
			break;
		case 5:
			// buzzer game #2
			this->current_game_mode = mode_instructions;
			this->game_type = GAME_BUZZER;
			this->buzzerscore = 0;
			this->playerid[0] = 0;
			this->playerid[1] = randbetween(0, NUM_ADDL_PLAYERS - 1) + 1;
			do
				{
				this->playerid[2] = randbetween(0, NUM_ADDL_PLAYERS - 1) + 1;
				} while (this->playerid[2] == this->playerid[1]);
			this->buzzersimlog.open("buzzersim.csv");
			this->buzzersimlog << "1/100s response,Called,Score,1/100s DelayAfterSpeech" << endl;
			this->ForceRedraw(true);
			break;

		case 7:
			this->current_game_mode = mode_specialtyselect;
			this->game_specialty = true;
			this->game_type = GAME_JEOPARDY;
			this->game_hard = false;
			this->tick_start = ticks;
			this->typebuffer[0] = 0;
			this->PrepareToAcceptInput();
			this->ForceRedraw(true);
			break;
		}
		break;

	case mode_buzzer_params:
		this->iclue.clear();
		this->ndone = 0;
		this->voiced = -1;
		this->current_game_mode = mode_buzzer_clue;
		this->tick_start = ticks;
		clback = this;
		buzzsc.nattempt = 0;
		buzzsc.ntotal = 0;
		buzzsc.ncall = 0;
		buzzsc.nearly = 0;
		buzzsc.n10 = 0;
		buzzsc.n5 = 0;
		buzzsc.n15 = 0;
		buzzsc.tdelay = 0;
		this->BuzzerClue();
		this->ForceRedraw(true);
		break;

	case mode_instructions:
		this->clue_bmp = nullptr;
		this->namecall = -1;
		this->catreveal = -1;
		this->tick_start = ticks;
		switch (this->game_type) {
		case GAME_BUZZER:
			this->current_game_mode = mode_buzzer_params;
			break;
		default:
			clback = this;
			this->current_game_mode = mode_category_reveal;
			break;
		}
		this->ForceRedraw(true);
		break;
	}
}


void Jeopardy::RButtonDown() {
	this->rdown = true;

	if (this->current_game_mode == mode_clue)
		this->ForceRedraw();
}

void Jeopardy::RButtonUp() {
	this->rdown = false;

	if (this->current_game_mode == mode_clue)
		this->ForceRedraw();
}

bool isalphanum(char c)
{
	if (isalpha(c))
		return(true);
	if (isdigit(c))
		return(true);

	return(false);
}

bool CheckThis(char *a, const char *s)
{
	char *res;
	const char *comp;

	res = a;
	comp = s;
	forever {
		if (!(*comp))
			break;
		if (!(*res))
			return(false);
		if (!isalphanum(*res)) {
			res++;
			continue;
		}
		if (!isalphanum(*comp)) {
			comp++;
			continue;
		}

		if (toupper(*res) != toupper(*comp))
			return(false);

		++res;
		++comp;
	}

	strcpy(a, res);
	return(true);
}

// some grammatically incorrect and a few misspellings are accepted for quick typists tripping over the keys
static const char* quesstart[] = {
	"WHAT IS ", "WHAT WAS ", "WHAT ARE ", "WHAT WERE ", "WHO IS ", "WHO OR WHAT IS ", "WHO WAS ", "WHO WERE ",
	"WHO ARE ", "WHOM IS ", "WHOM WAS ", "WHOM ARE ", "WHOM WERE ", "WHERE IS ", "WHERE WAS ", "WHERE WERE ",
	"WHERE ARE ", "WHY IS ", "WHY WAS ", "WHY WERE ", "WHY ARE ", "WHEN IS ", "WHEN WAS ", "WHEN ARE ", "WHEN WERE ",
	"HOW IS ", "HOW WAS ", "HOW ARE ", "HOW WERE ", "HOW MANY ARE ", "HWAT IS", "HWAT ARE", "WAHT IS", "WAHT ARE",
	"WHAT SI", "HWO IS ", "HWO ARE "
	};

bool FormOfAQuestion(char *a) {
	char *w;

	w = a;
	while (*w && isspace(*w))
		w++;

	for (const auto qs : quesstart) {
		if (CheckThis(w, qs))
			return true;
	}

	return(false);
}


void RemoveWord(char *s, const char *w) {
	char buf[16];
	char *x;
	bool rem;

	sprintf(buf, "%s ", w);
	if (!strncmp(s, buf, strlen(buf))) {
		// remove at the beginning
		strcpy(s, s + strlen(buf));
	}

	// remove from the middle
	sprintf(buf, " %s ", w);

	while ((x = strstr(s, buf)) != NULL) {
		strcpy(x, x + strlen(buf));
	}

	// allow to remain at the end
}

void RemoveNonAlphanum(char *a) {
	char *w;
	char *x;

	w = a;
	x = a;
	while (*x) {
		if ((*x >= '0' && *x <= '9') || (*x >= 'A' && *x <= 'Z')) {
			*w = *x;
			++w;
		}
		++x;
	}
	*w = 0;
}

bool SubStrMatch(char *a1, char *a2, int len) {
	/* Transposed letters/one-off typoes: OK. Grossly misspelt or completely different answer: not OK. */
	int c[36][2];
	int e;

	for (e = 0; e < 36; ++e) {
		c[e][0] = 0;
		c[e][1] = 0;
	}

	for (e = 0; e < len; ++e) {
		int i;

		if (a1[e] >= '0' && a1[e] <= '9')
			i = 26 + a1[e] - '0';
		else
			i = a1[e] - 'A';
		c[i][0]++;

		if (a2[e] >= '0' && a2[e] <= '9')
			i = 26 + a2[e] - '0';
		else
			i = a2[e] - 'A';
		c[i][1]++;
	}

	int diff;		// what's the ____?
	for (e = 0, diff = 0; e < 36; ++e)
		diff += abs(c[e][0] - c[e][1]);

	if (diff * 4 > len)
		return(false);

	return(true);
}

bool CmpAnswers(char *a1, char *a2) {
	// a1 is the shorter answer here
	char buf[1024];
	char *w;
	char *wm;
	int l;

	// handle multiple answers separated by slashes
	w = strchr(a2, '/');
	if (w) {
		char *na1, *na2;

		*w = 0;
		if (strlen(a2) < strlen(a1)) {
			na1 = a2;
			na2 = a1;
		} else {
			na1 = a1;
			na2 = a2;
		}
		if (CmpAnswers(na1, na2))
			return(true);

		if (strlen(w + 1) < strlen(a1)) {
			na1 = w + 1;
			na2 = a1;
		} else {
			na1 = a1;
			na2 = w + 1;
		}

		if (CmpAnswers(na1, na2))
			return(true);

		*w = '/';
	}

	w = a2;
	wm = a2 + strlen(a2);
	l = strlen(a1);
	buf[l] = 0;

	if (l < 4) {
		// fewer than 4 characters: accept an exact match only
		if (!strcmp(a1, a2))
			return(true);
		return(false);
	}

	while (w + l <= wm) {
		strncpy(buf, w, l);
		if (SubStrMatch(a1, buf, l))
			return(true);
		w++;
	}
	
	return(false);
}

const char* SkipQuestion(const char* a) {
	for (const auto qs : quesstart) {
		size_t l = strlen(qs);
		if (!strncmp(a, qs, l))
			return a + l;
	}
	return a;
}

bool ExactAnswer(const char* a1, const char* a2) {
	a1 = SkipQuestion(a1);
	return strcmp(a1, a2) == 0;
}

bool AcceptAnswer(const char *a1, const char *a2, int game_type) {
	char na1[1024];
	char na2[1024];
	bool art;
	bool ret;

	art = false;

	forever {
		strcpy(na1, a1);
		strcpy(na2, a2);
		__strupr(na1);
		__strupr(na2);

		if (!FormOfAQuestion(na1)) {
			if (game_type == GAME_JEOPARDY || game_type == GAME_CHALLENGE || game_type == GAME_JEOPARDY3)
				return(false);
			// not necessary to answer in the form of a question in the contestant quiz
		}
		if (ExactAnswer(na1, na2)) {
			return true;
		}

		// remove articles and some other potentially troublesome words
		if (!art) {
			RemoveWord(na1, "A");
			RemoveWord(na2, "A");
			RemoveWord(na1, "AN");
			RemoveWord(na2, "AN");
			RemoveWord(na1, "THE");
			RemoveWord(na2, "THE");
			RemoveWord(na1, "AND");
			RemoveWord(na2, "AND");
			RemoveWord(na1, "OF");
			RemoveWord(na2, "OF");
		}
		RemoveNonAlphanum(na1);
		RemoveNonAlphanum(na2);
		if (ExactAnswer(na1, na2)) {
			return true;
		}
		if (art)
			break;

		if (strlen(na1) > 0 && strlen(na2) > 0)
			break;

		art = true;
	}

	// check for exact match, to skip the comparisons below
	if (!strcmp(na1, na2))
		return(true);

	// again, check for a bingo
	if (!strcmp(na1, na2))
		return(true);

	// we'll allow substrings for cases like giving last names only, one of multiple possible answers, etc.
	if (strlen(na1) < strlen(na2)) {
		ret = CmpAnswers(na1, na2);
		if (!ret) {
			// Add an "S" and check again, just in case it's a short clue and user entered singular/plural where 
			// plural/singular was expected -- we want to accept it in that case 
			strcat(na1, "S");
			ret = CmpAnswers(na1, na2);
		}
		return(ret);
	}

	ret = CmpAnswers(na2, na1);
	if (!ret && strlen(na2) < strlen(na1)) {
		// as above: try adding an S and see if it makes it better
		strcat(na2, "S");
		ret = CmpAnswers(na2, na1);
	}

	return(ret);
}

void Jeopardy::TurnBuzzersOff(void) {
	canring[0] = false;
	canring[1] = false;
	canring[2] = false;
}

bool Jeopardy::AllBuzzersOff(void) {
	return (!canring[0] && !canring[1] && !canring[2]);
}

void Jeopardy::CheckAIAnswer(bool daily_double) {
	int damount;
	std::string resp = current_generation_str(), ans;

	damount = (this->s_a + 1) * 200;
	if (this->round == 1)
		damount += damount;	// Double Jeopardy!
	if (daily_double)
		damount = this->ddwagers;

	if ((this->game_type != GAME_JEOPARDY && this->game_type != GAME_JEOPARDY3) || this->round == 2) {
		ans = this->final_question;
	} else {
		ans = this->accept;
	}

	correctok = true;

	//std::cout << "Checking AI answer: " << resp << "\nI expect the answer: " << ans << "\n";
	if (AcceptAnswer(resp.c_str(), ans.c_str(), this->game_type)) {
		// AI is correct, they get control
		// for FJ! we adjust the score in DrawFJReveal()
		if (this->round != 2) {
			PlayAlexResponse(true);
			oppscore[op_ringin] += damount;
			if (daily_double) {
				this->current_game_mode = mode_opponent_dd_question;
				this->ForceRedraw();
			} else {
				clback = nullptr;
				this->inputstart = false;
				this->answerchecked = true;
				this->showanswer = true;
				this->showamount = true;
				this->wager = -1;
				this->control = op_ringin + 1;
			}
			TurnBuzzersOff();
		} else {
			// for FJ!, communicate the response via flag
			lastqright = true;
		}
	} else {
		// AI player is incorrect, they can no longer ring in and the clue continues (unless we're a Daily Double)
		// for FJ! we adjust the score in DrawFJReveal()
		ai_judge = op_ringin;
		ai_judgeamt = damount;
		if (this->round != 2) {
			PlayAlexResponse(false);
			oppscore[op_ringin] -= damount;
			this->thinksknows[0] = false;
			this->thinksknows[1] = false;
			if (daily_double) {
				this->current_game_mode = mode_opponent_dd_question;
				this->ForceRedraw();
			} else {
				clback = this;
				if (AllBuzzersOff()) {
					this->inputstart = false;
					this->answerchecked = true;
					this->showanswer = true;
					this->showamount = true;
					this->wager = -1;			
				} else {
					//this->inputstart = false;
					this->timesound = false;
					this->tick_start = ticks;
					this->drawtimer = false;
					this->current_game_mode = mode_clue;
					PrepareToAcceptInput();
				}
			}
		} else {
			// for FJ!, communicate the response via flag
			lastqright = false;
		}
	}
}

void Jeopardy::CheckAnswer(void) {
	int damount;
	char *ans;

	damount = (this->s_a + 1) * 200;
	if (this->round == 1)
		damount += damount;	// Double Jeopardy!

	if ((this->game_type != GAME_JEOPARDY && this->game_type != GAME_JEOPARDY3) || this->round == 2) {
		ans = this->final_question;
	} else {
		ans = this->accept;
	}
	ai_judge = -1;
	ai_judgeamt = 0;
	correctok = true;

	response.type = gamestats.type;
	response.date = gamestats.date;
	response.time = gamestats.time;
	if (this->game_type != GAME_QUIZ)
		response.catid = this->cats[this->s_c + this->round * 6];
	response.value = (this->s_a + 1) * 200;
	if (this->round > 0)
		response.value *= 2;
	response.round = this->round;
	if (this->ddsel)
		response.ringin = 2;
	else
		response.ringin = 1;
	strncpy(response.response, this->typebuffer, 256);
	response.added = false;

	if (AcceptAnswer(this->typebuffer, ans, this->game_type)) {
		response.right = 1;
		lastqright = true;
		this->control = 0;
		if (this->game_type == GAME_JEOPARDY || this->game_type == GAME_JEOPARDY3) {
			if (!(this->game_type == GAME_JEOPARDY3 && this->round == 2))
				PlayAlexResponse(true);
			if (this->wager >= 0) {
				this->playerscore += this->wager;
				this->judgedock = -this->wager;
			} else {
				this->playerscore += damount;
				this->judgedock = -damount;
			}
			this->pcoryat += damount;
			this->dcoryat = -damount;
		} else {
			this->nright++;
			clback = this;
			this->tick_start = ticks;
			this->current_game_mode = mode_category_reveal;
			this->ForceRedraw(true);
		}
	} else {
		response.right = 0;
		lastqright = false;
		if (this->game_type == GAME_JEOPARDY || this->game_type == GAME_JEOPARDY3) {
			if (!(this->game_type == GAME_JEOPARDY3 && this->round == 2))
				PlayAlexResponse(false);
			if (this->wager >= 0) {
				this->playerscore -= this->wager;
				this->judgedock = this->wager;
			} else {
				this->playerscore -= damount;
				this->judgedock = damount;
			}
			this->pcoryat -= damount;
			this->dcoryat = damount;
		} else {
			clback = this;
			this->tick_start = ticks;
			this->current_game_mode = mode_category_reveal;
			this->ForceRedraw(true);
		}
	}
	if (this->round < 2)
		this->showanswer = true;
	
	if (!(this->game_type == GAME_JEOPARDY3 && this->round == 2)) {
		this->showamount = true;
		this->wager = -1;
		this->ForceRedraw();
	}
}

void Jeopardy::InitClue3Player(void) {
	int e;

	for (e = 0; e < 3; ++e)
		this->canring[e] = true;
	if (randbetween(0, 99) < this->tkp[0][s_a] + tkpbonus(this->round, s_a))
		this->thinksknows[0] = true;
	else
		this->thinksknows[0] = false;

#ifdef RINGIN_DEPENDENT
	if (this->thinksknows[0] == false && randbetween(0, 99) >= dep100)
		this->thinksknows[1] = false;
	else if (randbetween(0, 99) < this->tkp[1][s_a] + tkpbonus(this->round, s_a))
		this->thinksknows[1] = true;
	else
		this->thinksknows[1] = false;
#else
	if (randbetween(0, 99) < this->tkp[1][s_a] + tkpbonus(this->round, s_a))
		this->thinksknows[1] = true;
	else
		this->thinksknows[1] = false;
#endif
	if (randbetween(0, 99) < this->akp[0][s_a])
		this->actuallyknows[0] = true;
	else
		this->actuallyknows[0] = false;
	if (randbetween(0, 99) < this->akp[1][s_a])
		this->actuallyknows[1] = true;
	else
		this->actuallyknows[1] = false;

	if (llama != nullptr) {
		// Both AI players think they know, but since we're using greedy sampling (or
		// low temperature), we'll clear both if one AI player answers incorrectly.
		this->thinksknows[0] = true;
		this->thinksknows[1] = true;
	}

	// for a Daily Double... use the probability ranges indicated from J! Archive, which hover around 65-75%
	if (this->ddsel) {
		if (randbetween(0, 99) < (65 + (randbetween(0, 10))))
			this->actuallyknows[0] = true;
		else
			this->actuallyknows[0] = false;
		if (randbetween(0, 99) < (65 + (randbetween(0, 10))))
			this->actuallyknows[1] = true;
		else
			this->actuallyknows[1] = false;
	}

	// initialize Daily Double wager
	this->ddwagers = 0;
	if (this->control > 0)
		this->ddwagers = this->OpponentDDWager();
}


int Jeopardy::WhoRings(bool player) {
	vector<int> ik;
	int i;

	ik.clear();

	if (player && this->canring[0])
		ik.push_back(0);
	if (this->thinksknows[0] && this->canring[1])
		ik.push_back(1);
	if (this->thinksknows[1] && this->canring[2])
		ik.push_back(2);

	if (ik.size() == 0)
		return(-1);
	i = randbetween(0, ik.size() - 1);

	return(ik[i]);
}


void Jeopardy::OpponentRingin(int ip) {
	int damount;
	int tk;
	int c;
	int voiceid;

	damount = (this->s_a + 1) * 200;
	if (this->round == 1)
		damount += damount;	// Double Jeopardy!

	this->canring[ip] = false;

	this->drawtimer = false;
	this->timesound = false;
	if (this->llama == nullptr) {
		clback = nullptr;
	}
	op_ringin = ip - 1;

	// have Alex call the player's name
	//voiceid = 
	soundfx.play_sfx(this->sound_fx[sfx_playerchris + playerid[ip]]);

	if (this->llama == nullptr && !_timeout) {
		if (this->actuallyknows[ip - 1])
			this->control = ip;
		PlayAlexResponse(this->actuallyknows[ip - 1]);
	}

	if (this->llama != nullptr) {
		// AI answer. Begin generation!
		std::string ourcat;
		if (this->s_c > 5)
			ourcat = category_names[this->s_c - 6];
		else
			ourcat = category_names[this->s_c];
		request_generation(llama, ourcat, cur_clue_text);
		current_game_mode = mode_aiplayer_answers;
	} else if (this->actuallyknows[ip - 1]) {
		// answer questioned correctly: give player money, end of clue
		oppscore[ip - 1] += damount;
		this->inputstart = false;
		this->answerchecked = true;
		this->showanswer = true;
		this->showamount = true;
		this->wager = -1;
		this->control = ip;
	} else {
		// answer questioned incorrectly: dock player money, clue is still up for grabs
		oppscore[ip - 1] -= damount;
		this->inputstart = false;
		this->timesound = false;
		this->tick_start = ticks;
		this->drawtimer = false;
		clback = this;
	}
}


vector<char *> sp_responses;
vector<int> *sp_cats;

int cluecompare(char *s1, char *s2) {
	char c1[512], c2[512];
	char *w;
	char *x;

	w = s1;
	x = c1;
	forever {
		if (!strncmp(w, "<i>", 3)) {
			w += 3;
			continue;
		}
		if (!strncmp(w, "</i>", 4)) {
			w += 4;
			continue;
		}
		if (!(*w)) {
			*x = 0;
			break;
		}
		if (isalpha(*(unsigned char *)w)) {
			*x = *w;
			++x;
		}
		++w;
	}
	w = s2;
	x = c2;
	forever {
		if (!strncmp(w, "<i>", 3)) {
			w += 3;
			continue;
		}
		if (!strncmp(w, "</i>", 4)) {
			w += 4;
			continue;
		}
		if (!(*w)) {
			*x = 0;
			break;
		}
		if (isalpha(*(unsigned char *)w)) {
			*x = *w;
			++x;
		}
		++w;
	}

	return (__stricmp(c1, c2));
}

static int specialty_callback(void *v, int ncols, char **cols, char **colname) {
	int e;
	int catid;
	int s;

	catid = atoi(cols[0]);

	s = sp_responses.size();
	for (e = 0; e < s; ++e) {
		if (!cluecompare(cols[1], sp_responses[e])) {
			(*sp_cats).push_back(catid);
			break;
		}
	}

	return(0);
}


void Jeopardy::FindSpecialties(void) {
	ifstream i;
	char radd[128];
	char line[1024];
	char *w;
	int e;

	strncpy(specialty, this->typebuffer, 128);
	__strupr(specialty);
	specialtycats.clear();
	sp_cats = &this->specialtycats;

	sp_responses.clear();
	i.open("specialties.txt");
	forever {
		i.getline(line, 1024, '\n');
		if (i.eof())
			break;
		w = strchr(line, '\t');
		if (!w)
			continue;
		*w = 0;
		if (strstr(w + 1, specialty) != NULL) {
			w = new char [strlen(line) + 1];
			strcpy(w, line);
			sp_responses.push_back(w);
		}
	}
	i.close();
	i.clear();

	sqlite3_exec(this->db, "SELECT catid,question FROM Clue;", specialty_callback, NULL, NULL);

	std::sort(this->specialtycats.begin(), this->specialtycats.end());

	// remove duplicate categories
	for (e = 1; e < this->specialtycats.size(); ++e) {
		if (this->specialtycats[e] == this->specialtycats[e - 1]) {
			this->specialtycats.erase(this->specialtycats.begin() + e);
			--e;
		}
	}
	// remove categories without enough clues 
	for (e = 0; e < this->specialtycats.size(); ++e) {
		if (cluecount[this->specialtycats[e]] < 5) {
			this->specialtycats.erase(this->specialtycats.begin() + e);
			--e;
		}
	}
}


void Jeopardy::KeyUp(int nChar) {
	int l;
	bool ringin;
	char add[4];
	bool userentry = false;

	if (this->judgedock >= 0 && (nChar == 'j' || nChar == 'J') && correctok) {
		// Judge button: correct human player's score
		response.right = 1;
		this->playerscore += (this->judgedock + this->judgedock);
		this->pcoryat += (this->dcoryat + this->dcoryat);
		//this->judgedock = -1;
		//this->dcoryat = -1;
		this->judgedock = -this->judgedock;
		this->dcoryat = -this->dcoryat;
		this->PlayAlexResponse(true);
		this->ForceRedraw(true);
		this->control = 0;
		userentry = true;
		return;
	} else if (ai_judge >= 0 && ai_judgeamt >= 0 && (nChar == 'j' || nChar == 'J') && correctok) {
		// Judge button: correct AI player's score
		this->oppscore[ai_judge] += ai_judgeamt;
		this->oppscore[ai_judge] += ai_judgeamt;
		this->PlayAlexResponse(true);
		// also give the computer control
		if (mode_clue == current_game_mode) {
			this->showanswer = true;
			this->drawtimer = false;
			TurnBuzzersOff();
			control = ai_judge + 1;
			this->ForceRedraw();
		}
		ai_judge = -1;
		ai_judgeamt = 0;
	} else if (this->judgedock < -1 && (nChar == '-' || nChar == '_') && correctok) {
		response.right = 0;
		this->playerscore += (this->judgedock + this->judgedock);
		this->pcoryat += (this->dcoryat + this->dcoryat);
		this->judgedock = -this->judgedock;
		this->dcoryat = -this->dcoryat;
		this->PlayAlexResponse(false);
		this->ForceRedraw(true);
		userentry = true;
		return;
	} else if (ai_judge >= 0 && ai_judgeamt >= 0 && (nChar == '-' || nChar == '_') && correctok) {
		this->oppscore[ai_judge] -= ai_judgeamt;
		this->oppscore[ai_judge] -= ai_judgeamt;
		this->PlayAlexResponse(false);
		this->ForceRedraw(true);
		ai_judge = -1;
		ai_judgeamt = 0;
	}

	if (this->current_game_mode == mode_fj3rd) {
		_adjsc = true;
		this->current_game_mode = mode_fj2nd;
		this->ForceRedraw();
		return;
	}
	if (this->current_game_mode == mode_fj2nd) {
		_adjsc = true;
		this->current_game_mode = mode_fj1st;
		this->ForceRedraw();
		return;
	}
	if (this->current_game_mode == mode_fj1st) {
		this->current_game_mode = mode_clue;
		this->tick_start = ticks;
		this->CreateClueBitmap(this->final_question, false);
		this->showamount = true;
		this->fjdone = true;
		this->ForceRedraw();
		return;
	}
	if (this->current_game_mode == mode_opponent_dd_wager) {
		if (this->llama != nullptr) {
			std::string ourcat;
			if (this->s_c > 5)
				ourcat = category_names[this->s_c - 6];
			else
				ourcat = category_names[this->s_c];
			request_generation(llama, ourcat, cur_clue_text);
			current_game_mode = mode_aiopponent_dd_answer;
		} else {
			this->current_game_mode = mode_opponent_dd_answer;
		}
		this->ForceRedraw();
		return;
	}
	if (this->current_game_mode == mode_buzzer_params)
		{
		switch (nChar) {
		case '+':
		case '=':
			this->delayv++;
			this->ForceRedraw();
			break;
		case '-':
		case '_':
			if (this->delayv > 0)
				this->delayv--;
			this->ForceRedraw();
			break;
		case ',':
		case '<':
			if (this->driftv > 0)
				this->driftv--;
			this->ForceRedraw();
			break;
		case '.':
		case '>':
			this->driftv++;
			this->ForceRedraw();
			break;
		}
		return;
	}
	if (this->current_game_mode == mode_opponent_dd_answer) {
		// have to combine tkp and akp to get the probability correct, given we DIDN'T ring in... 
		if ((RandPercent(this->tkp[this->control - 1][this->s_a] + tkpbonus(this->round, this->s_a)) &&
			(RandPercent(this->akp[this->control - 1][this->s_a])))) {
			this->oppscore[this->control - 1] += this->ddwagers;
			PlayAlexResponse(true);
		} else {
			this->oppscore[this->control - 1] -= this->ddwagers;
			PlayAlexResponse(false);
		}
		this->current_game_mode = mode_opponent_dd_question;
		this->ForceRedraw();
		return;
	}
	if (this->current_game_mode == mode_opponent_dd_question) {
		if (!this->CheckRound()) {
			this->current_game_mode = mode_board;
			this->BoardControl();
		}
		this->ForceRedraw(true);
		return;
	}
	if (this->current_game_mode == mode_introduce_contestants) {
		this->current_game_mode = mode_category_reveal;
		this->RoundBegin();
		this->ForceRedraw(true);
		return;
	}

	if (this->current_game_mode == mode_game_end) {
		if (this->place == 1) {
			if (!this->CheckRound()) {
				this->current_game_mode = mode_board;
				this->BoardControl();
			}
		} else {
			this->gamewon = 0;
			this->winnings = 0;
			this->CheckRound();
			this->current_game_mode = mode_main_menu;
		}
		this->ForceRedraw(true); 
		return;
	}

#if 0
	if (this->current_game_mode == mode_buzzer_clue)
		{
		if (!this->lightson)
			this->hsec = -999;
		else
			this->hsec = ticks - (this->voiced + this->mdelay); 
#if 0
		this->hsec = (ticks - this->tick_start) - this->readinglen;
#endif
		this->showtime = true;
		this->ForceRedraw(false);
		return;
		}
#endif

	if (this->current_game_mode == mode_specialtyselect) {
		l = strlen(this->typebuffer);
		if (nChar == 8 /* backspace */) {
			if (l > 0)
				this->typebuffer[l - 1] = 0;
		} else if (nChar == 13 /* enter */) {
			this->FindSpecialties();

			if (this->specialtycats.size() <= 12) {
				this->current_game_mode = mode_main_menu;
				this->ForceRedraw(true);
			} else {
				this->current_game_mode = mode_category_reveal;
				this->game_type = GAME_JEOPARDY;
				this->RoundBegin();
				this->ForceRedraw(true);
			}
			return;
		} else {
			if (l < 1023 && !this->timesound) {
				if (nChar == '/')
					strcat(this->typebuffer, "?");
				else if (isspace(nChar) && ringin)
					/* ringing in with spacebar shouldn't add to the type buffer */ ;
				else {
					sprintf(add, "%c", (char) nChar);
					if (isspace(nChar)) {
						int e;
						for (e = 0; this->typebuffer[e]; ++e)
							if (!isspace(this->typebuffer[e]))
								break;
						if (this->typebuffer[e])
							strcat(this->typebuffer, add);
					} else {
						strcat(this->typebuffer, add);
					}
				}
			}
		}
		if (!this->drawtimer)
			this->ForceRedraw();

		return;
	}

	ringin = false;
	if (this->isgame && (this->current_game_mode == mode_clue || this->current_game_mode == mode_wager)) {
		ringin = false;
		if (!this->inputstart && this->game_type == GAME_JEOPARDY3 && !this->ddsel && this->round < 2 && !answerchecked) {
			// attempt to ring in
			int ip;
			ip = WhoRings(true);
			if (ip < 0)
				return;
			if (ip > 0) {
				// somebody beat us to the buzzer
				response.ringin = 0;
				this->OpponentRingin(ip);
				this->ForceRedraw();
				return;
			} else {
				// Chris rings in, hooray
				soundfx.play_sfx(this->sound_fx[sfx_playerchris]);
				response.ringin = 1;
			}
		}

		if (!this->inputstart) {
			this->inputstart = true;
			if (this->current_game_mode == mode_clue && this->round < 2) {
				this->timesound = false;
				this->tick_start = ticks;
				if (!showanswer)
					this->drawtimer = true;
				this->tcount = 0;
				clback = this;
			}
			if (nChar >= '0' && nChar <= '9' && this->current_game_mode != mode_wager) {
				// rang in with the buzzer: don't add the numeric buzzer key to the answer buffer
				return;
			}
		}

		l = strlen(this->typebuffer);

		if (nChar == 8 /* backspace */) {
			if (l > 0)
				this->typebuffer[l - 1] = 0;
		} else if (nChar == 13 /* enter */) {
			if (this->current_game_mode == mode_clue) {
				userentry = true;
				switch (this->game_type) {
				case GAME_QUIZ:
					this->drawtimer = false;
					clback = nullptr;
					this->CheckAnswer();
					this->answerchecked = true;
					this->lastq = this->final_question;
					break;
				case GAME_JEOPARDY:
				case GAME_JEOPARDY3:
					if (this->round < 2 && !this->answerchecked) {
						// enter the answer
						this->drawtimer = false;
						clback = NULL;
						this->CheckAnswer();
						this->answerchecked = true;
					}
					break;
				}
			} else {
				if (this->ValidateWager()) {
					if (this->round < 2) {
						int sid;
						if (!this->wasleading && this->willlead)
							sid = sfx_wager9;	// "For the lead, here is the clue..."
						else if (this->istruedd)
							sid = sfx_wager8;	// "All right, a true Daily Double..."
						else {
							int wsfx;
							wsfx = randbetween(0, 7);
							if (wsfx == 7)
								wsfx += 2;	// wager10
							sid = sfx_wager1 + wsfx;
						}
						soundfx.play_sfx(this->sound_fx[sid]);
						this->current_game_mode = mode_clue;
						clback = NULL;
						this->tick_start = ticks;
						this->PrepareToAcceptInput();
					} else {
						// Show the Final Jeopardy! clue
						this->showanswer = false;
						this->CreateClueBitmap(this->final_clue, false);
						soundfx.play_sfx(this->sound_fx[sfx_reveal]);
						this->current_game_mode = mode_clue;
						clback = this;
						this->tick_start = ticks;
						this->PrepareToAcceptInput();
					}
					this->ForceRedraw();
				} else {
					// invalid wager, clear out the buffer
					this->typebuffer[0] = 0;
				}
			}
		} else {
			if (l < 1023 && !this->timesound) {
				if (nChar == '/')
					strcat(this->typebuffer, "?");
				else if (isspace(nChar) && ringin)
					/* ringing in with spacebar shouldn't add to the type buffer */ ;
				else {
					sprintf(add, "%c", char(nChar));
					if (isspace(nChar)) {
						int e;
						for (e = 0; this->typebuffer[e]; ++e)
							if (!isspace(this->typebuffer[e]))
								break;
						if (this->typebuffer[e])
							strcat(this->typebuffer, add);
					} else {
						strcat(this->typebuffer, add);
					}
				}
			}
		}
		if (!this->drawtimer)
			this->ForceRedraw();
	}
	if ((this->isgame && !userentry && (this->current_game_mode == mode_clue || this->current_game_mode == mode_aiplayer_answers) && showanswer && round != 2) ||
		(this->isgame && AllBuzzersOff() && (this->current_game_mode == mode_clue || this->current_game_mode == mode_aiplayer_answers) && round != 2)) {
		if (!this->CheckRound()) {
			this->current_game_mode = mode_board;
			this->BoardControl();
		}
		ForceRedraw(true);
	}
}

struct cluedata {
	char *answer;
	char *question;
	char *catname;
	int date;
	int count;
	bool final;
	int person;
};

vector<cluedata> cluestats;
static int all_clue_callback(void *v, int ncols, char **cols, char **colname) {
	/*
		0	catid
		1	catround
		2	catcol
		3	gameid
		4	date
		5	catname
		6	catid
		7	money
		8	answer
		9	question
	*/
	cluedata cd;

	cd.count = 0;
	cd.final = false;
	cd.person = 0;
	cd.answer = new char [strlen(cols[8]) + 1];
	strcpy(cd.answer, cols[8]);
	NormalizeStr(cd.answer);
	cd.question = new char [strlen(cols[9]) + 1];
	strcpy(cd.question, cols[9]);
	NormalizeStr(cd.question);
	cd.catname = new char [strlen(cols[5]) + 1];
	strcpy(cd.catname, cols[5]);
	NormalizeStr(cd.catname);
	cd.date = atoi(cols[4]);
	if (atoi(cols[1]) == 2)
		cd.final = true;

	cluestats.push_back(cd);

	return(0);
}

vector<int> clueindices;

void ClueToLetArray(char *clue, char *arr) {
	char c;

	if (!strncmp(clue, "The ", 4))
		clue += 4;
	if (!strncmp(clue, "the ", 4))
		clue += 4;
	if (!strncmp(clue, "A ", 2))
		clue += 2;
	if (!strncmp(clue, "a ", 2))
		clue += 2;
	if (!strncmp(clue, "An ", 3))
		clue += 3;
	if (!strncmp(clue, "an ", 3))
		clue += 3;

	while (*clue) {
		c = *clue;
		c = toupper(c);
		if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
			*arr = c;
			++arr;
		}
		++clue;
	}
	*arr = 0;
}

bool cluestats_sort2(const cluedata cd1, const cluedata cd2) {
	char a1[512];
	char a2[512];

	ClueToLetArray(cd1.question, (char *)a1);
	ClueToLetArray(cd2.question, (char *)a2);

	return (strcmp(a1, a2) < 0);
}

bool cluestats_sort(const int i1, const int i2) {
	if (cluestats[i1].count == cluestats[i2].count)
		return (cluestats_sort2(cluestats[i1], cluestats[i2]));
	return (cluestats[i1].count > cluestats[i2].count);
}

void LetterArray(char *str, int *arr) {
	char c;
	int e;
	
	for (e = 0; e < 36; ++e)
		arr[e] = 0;

	if (!strncmp(str, "The ", 4))
		str += 4;
	if (!strncmp(str, "the ", 4))
		str += 4;
	if (!strncmp(str, "A ", 2))
		str += 2;
	if (!strncmp(str, "a ", 2))
		str += 2;
	if (!strncmp(str, "An ", 3))
		str += 3;
	if (!strncmp(str, "an ", 3))
		str += 3;

	while (*str) {
		c = *str;
		c = toupper(c);
		if (c >= 'A' && c <= 'Z')
			arr[c - 'A']++;
		else if (c >= '0' && c <= '9')
			arr[(c - '0') + 26]++;
		str++;
	}
}

bool AcceptExact(char *a1, char *a2) {
	int c1[36];
	int c2[36];
	int e;

	LetterArray(a1, (int *)c1);
	LetterArray(a2, (int *)c2);

	for (e = 0; e < 36; ++e)
		if (c1[e] != c2[e])
			return(false);

	return(true);
}

void OutputStrCSV(ostream &o, char *str) {
	o << "\"";
	while (*str) {
		if (*str == '\"')
			o << (*str);
		o << (*str);
		str++;
	}
	o << "\"";
}

void OutputStrNonewline(ostream &o, char *str) {
	while (*str) {
		if (*str != '\n' && *str != '\r')
			o << (*str);
		else
			o << " ";
		str++;
	}
}

char _dbuf[16];
const char *FormatAsDate(int d) {
	sprintf(_dbuf, "%02d/%02d/%d",
		(d / 100) % 100,
		d % 100,
		d / 10000);
	return (_dbuf);
}


void Jeopardy::KeyDown(int keycode) {
	if (this->current_game_mode == mode_buzzer_clue) { // && this->hsec < -999) {
		if (!this->lightson) {
#if 0
			int vpos;

			vpos = voice_get_position(this->voicen);

			if (vpos < 0)
				{
				this->hsec = ticks - (this->voiced + this->mdelay);
				}
			else
				{
				vpos = this->voicet - vpos;
				this->hsec = (vpos * 100) / this->voicef;
				this->hsec += this->mdelay;
				this->hsec = -this->hsec;
				}
#endif				
		} else {
			this->hsec = ticks - (this->voiced + this->mdelay); 
		}
		if (this->hsec_first == -9999) {
			this->hsec_first = this->hsec;

			if (this->buzzerscore >= 0) {
				// okay, did we ring in?
				bool ringin;
				int prob100;

				buzzsc.nattempt++;
				if (this->hsec_first < 0)
					buzzsc.nearly++;
				else {
					if (this->hsec_first <= 5)
						buzzsc.n5++;
					if (this->hsec_first <= 10)
						buzzsc.n10++;
					if (this->hsec_first <= 15)
						buzzsc.n15++; 	
				}

				buzzersimlog << this->hsec_first << ",";
				ringin = false;	// if early, locked out and count as somebody else rings in 
				if (this->hsec_first >= 0) {
					if (this->hsec_first <= 10) {
						// 100% at 0.00s, 97% at 0.01s,... 70% at 0.10s
						prob100 = 100 - this->hsec_first * 3;
					} else {
						// 70% at 0.10s, 64% at 0.11s, 58% at 0.12s..., to a minimum of 10% at 0.20s
						prob100 = 70 - (this->hsec_first - 10) * 6;
						if (prob100 < 10)
							prob100 = 10;
					}
					if (RandPercent(prob100))
						ringin = true;		// yay!
				}
				this->tickcall = ticks + 70;
				if (this->tickcall > this->tick_start + this->readinglen + 300)
					this->tickcall = this->tick_start + this->readinglen + 250; 
				if (ringin) {
					buzzsc.ncall++;
					this->namecall = 0;
					this->buzzerscore++;
					buzzersimlog << "TRUE," << this->buzzerscore << ",";
				} else {
					this->namecall = 1 + randbetween(0, 1);
					buzzersimlog << "FALSE," << this->buzzerscore << ",";
				}
				buzzersimlog << this->mdelay  << endl; 
			}
		}
		this->showtime = true;
		this->ForceRedraw(false);
	}
}

void main_loop(Display* display, void* user_data) {
	static KeyLast kl(display);
	static Stopwatch sw;
	Jeopardy* game = (Jeopardy *) user_data;
	int mx = display->mouse_x(), my = display->mouse_y();

	if (kl.first()) {
		sw.start();
	}

	// first, update the timer
	/* Ticks should be the time in hundredths of a second since the start of the app. */
	int tl = ticks;
	ticks = sw.stop(UNIT_MILLISECOND) / 10;
	if (ticks != tl) {
		soundfx.update();
		if (clback != nullptr && !_timerlock) {
			clback->TimerCallback();
		}
	}

	// tell the game where the mouse is
	game->MouseMove(mx, my);

	// handle key up/down events
	for (int e = 8; e < 256; ++e) {
		if (kl.now_down(e)) {
			game->KeyDown(e);
		} else if (kl.now_up(e)) {
			game->KeyUp(e);
		}
	}

	// handle mouse button up/down events
	if (kl.mouse_now_down(MOUSEBUTTON_RIGHT)) {
		game->RButtonDown();
	}
	if (kl.mouse_now_up(MOUSEBUTTON_RIGHT)) {
		game->RButtonUp();
	}
	if (kl.mouse_now_up(MOUSEBUTTON_LEFT)) {
		game->LButtonUp();
	}

	// draw the screen
	assert(!is_null(game->screen));
	game->Draw();
	game->screen->blit(display->bitmap());

	// save the keyboard state
	kl.save(display);
}

int app_main() {
	Jeopardy jeopardy;
#ifdef CODEHAPPY_CUDA
	ArgParse ap;
	std::string model;
	double temp = 0.0;
	bool vanilla = false, twogpu = false;

	ap.add_argument("vanilla", type_none, "run in vanilla mode, with modeled computer opponents (no LLM answers)", &vanilla);
	ap.add_argument("model", type_string, "language model (in GGML/GGUF format, isn-tuned Llama 2 70B recommended)");
	ap.add_argument("temp", type_double, "temperature to use in model inference (default is 0.0, which indicates purely greedy sampling)", &temp);
	ap.add_argument("2gpu", type_none, "configuration for loading 70B model onto two 24GB VRAM GPUs", &twogpu);
	ap.ensure_args(argc, argv);
	if (ap.flag_present("model")) {
		model = ap.value_str("model");	
	}

	const char* model_path;
	if (vanilla || model.empty())
		model_path = nullptr;
	else
		model_path = model.c_str();

	jeopardy.Initialize(model_path, temp, twogpu, APP_WIDTH, APP_HEIGHT);
	std::thread gen_thread(generation_thread);
#else  // !CODEHAPPY_CUDA
	jeopardy.Initialize(nullptr, 0.0, false, APP_WIDTH, APP_HEIGHT);
#endif

	codehappy_main(main_loop, &jeopardy, APP_WIDTH, APP_HEIGHT);

	return 0;
}

