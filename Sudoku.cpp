#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "serialAtmega.h"
#include "spiAVR.h"
#include "irAVR.h"
#include <string.h>


//Pin Definitions
// #define CS_PIN PINB2    //PORTB 10
// #define RST_PIN PINB2   //PORTB 8
// #define DC_PIN PINB1    //PORTD 7
// #define MOSI_PIN PINB3  //PORTB 11
// #define SCK_PIN PINB5   //PORTB 13

//Command and Data macros
#define CS_LOW() (PORTB = SetBit(PORTB, 2, 0))
#define CS_HIGH() (PORTB = SetBit(PORTB, 2, 1))
#define DC_LOW() (PORTD = SetBit(PORTD, 7, 0))
#define DC_HIGH() (PORTD = SetBit(PORTD, 7, 1))
#define RST_LOW() (PORTB = SetBit(PORTB, 0, 0))
#define RST_HIGH() (PORTB = SetBit(PORTB, 0, 1))

int sudokuBoard[9][9] = {
    {5, 3, 0, 0, 7, 0, 0, 0, 0},
    {6, 0, 0, 1, 9, 5, 0, 0, 0},
    {0, 9, 8, 0, 0, 0, 0, 6, 0},
    {8, 0, 0, 0, 6, 0, 0, 0, 3},
    {4, 0, 0, 8, 0, 3, 0, 0, 1},
    {7, 0, 0, 0, 2, 0, 0, 0, 6},
    {0, 6, 0, 0, 0, 0, 2, 8, 0},
    {0, 0, 0, 4, 1, 9, 0, 0, 5},
    {0, 0, 0, 0, 8, 0, 0, 7, 9}
};

int correctAnswer[9][9] = {
    {0, 0, 4, 6, 0, 8, 9, 1, 2},
    {0, 7, 2, 0, 0, 0, 3, 4, 8},
    {1, 0, 0, 3, 4, 2, 5, 0, 7},
    {0, 5, 9, 7, 0, 1, 4, 2, 0},
    {0, 2, 6, 0, 5, 0, 7, 9, 0},
    {0, 1, 3, 9, 0, 4, 8, 5, 0},
    {9, 0, 1, 5, 3, 7, 0, 0, 4},
    {2, 8, 7, 0, 0, 0, 6, 3, 0},
    {3, 4, 5, 2, 0, 6, 1, 0, 0}
};

int userAnswer[9][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const int gridSize = 9;
const int cellSize = (130 - 20) / gridSize;
const int margin = (130 - (cellSize * gridSize)) / 2;

void HardwareReset() {
    RST_LOW();
    //PORTB = SetBit(PORTB, 0, 0);
    _delay_ms(200);
    RST_HIGH();
    //PORTB = SetBit(PORTB, 0, 1);
    _delay_ms(200);
}

void ST7735_writeCommand(char cmd) {
    CS_LOW();
    //PORTB = SetBit(PORTB, 2, 0);
    DC_LOW();
    //PORTD = SetBit(PORTD, 7, 0);
    SPI_SEND(cmd);
    CS_HIGH();
    //PORTB = SetBit(PORTB, 2, 1);
}

void ST7735_writeData(char data) {
    CS_LOW();
    //PORTB = SetBit(PORTB, 2, 0);
    DC_HIGH();
    //PORTD = SetBit(PORTD, 7, 1);
    SPI_SEND(data);
    CS_HIGH();
    //PORTB = SetBit(PORTB, 2, 1);
}

void ST7735_init() {
    HardwareReset();
    ST7735_writeCommand(0x01);  //software reset
    // PORTB = SetBit(PORTB, 2, 0);
    // PORTD = SetBit(PORTD, 7, 0);
    // SPI_SEND(0x01);
    _delay_ms(150);

    ST7735_writeCommand(0x11);  //sleep out
    //SPI_SEND(0x11);
    _delay_ms(200);

    ST7735_writeCommand(0x3A);
    //SPI_SEND(0x3A);

    ST7735_writeData(0x06);
    // PORTD = SetBit(PORTD, 7, 1);
    // SPI_SEND(0x06);
    _delay_ms(10);

    ST7735_writeCommand(0x29);  //display on
    // PORTD = SetBit(PORTD, 7, 0);
    // SPI_SEND(0x29);
    _delay_ms(200);
    //PORTB = SetBit(PORTB, 2, 1);

}

void ST7735_blackScreen() {
    ST7735_writeCommand(0x2A);
    ST7735_writeData(0);
    ST7735_writeData(0);
    ST7735_writeData(0);
    ST7735_writeData(130);

    ST7735_writeCommand(0x2B);
    ST7735_writeData(0);
    ST7735_writeData(0);
    ST7735_writeData(0);
    ST7735_writeData(130);

    ST7735_writeCommand(0x2C);


    for (unsigned short k = 0; k < 130*130; k++) {
        ST7735_writeData(0x00);
        ST7735_writeData(0x00);
        ST7735_writeData(0x00);
    }
}

void setAddrWindow(char x1, char y1, char x2, char y2) {
    ST7735_writeCommand(0x2A);  //Column addr set
    ST7735_writeData(0x00);
    ST7735_writeData(x1);
    ST7735_writeData(0x00);
    ST7735_writeData(x2);

    ST7735_writeCommand(0X2B);  //Row addr set
    ST7735_writeData(0x00);
    ST7735_writeData(y1);
    ST7735_writeData(0x00);
    ST7735_writeData(y2);

    ST7735_writeCommand(0x2C);
}

void drawPixel(char x, char y, char c1, char c2, char c3) {
    setAddrWindow(x, y, x+1, y+1);
    ST7735_writeData(c1);
    ST7735_writeData(c2);
    ST7735_writeData(c3);
}

void drawRect(char x, char y, char w, char h, char c1, char c2, char c3) {
    for(int z = x; z <= x + w; z++) {
        drawPixel(z, y, c1, c2, c3);
        drawPixel(z, y + h - 1, c1, c2 ,c3);
    }

    for (int z = y; z <= y + h; z++) {
        drawPixel(x, z, c1, c2, c3);
        drawPixel(x + w -1, z, c1, c2, c3);
    }
}

void drawCenteredNumber(int num, int row, int col, int cellSize, char c1, char c2, char c3) {
  // Basic bitmap representation for digits (for simplicity, here only 0-9)
  const uint8_t digits[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x62, 0x51, 0x49, 0x49, 0x46}, // 2
    {0x22, 0x41, 0x49, 0x49, 0x36}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x2F, 0x49, 0x49, 0x49, 0x31}, // 5
    {0x3E, 0x49, 0x49, 0x49, 0x32}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x26, 0x49, 0x49, 0x49, 0x3E}  // 9
  };

  int x = margin + col * cellSize + cellSize / 2 - 2;
  int y = margin + row * cellSize + cellSize / 2 - 3;

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 7; j++) {
      if (digits[num][i] & (1 << j)) {
        drawPixel(x + i, y + j, c1, c2, c3);
      }
    }
  }
}

void drawSudokuNumbers() {
  for (int row = 0; row < 9; row++) {
    for (int col = 0; col < 9; col++) {
      if (sudokuBoard[row][col] != 0) {
        drawCenteredNumber(sudokuBoard[row][col], row, col, cellSize, 255, 255, 255); // white color
      }
    }
  }
}

void drawSudokuBoard() {
    //Set Background to Black
    ST7735_blackScreen();

    //Draw Grid
    for (int i = 0; i <= gridSize; i++) {
        int lineThickness = (i % 3 == 0) ? 3 : 1; // Thicker lines for 3x3 sections
        
        // Vertical lines
        drawRect(margin + i * cellSize, margin, lineThickness, cellSize * gridSize, 0, 0, 255);
        
        // Horizontal lines
        drawRect(margin, margin + i * cellSize, cellSize * gridSize, lineThickness, 0, 0, 255);
    }

    //draw numbers
    drawSudokuNumbers();
}

void highlightCell(int row, int col, char c1, char c2, char c3) {
    int x = col * cellSize;
    int y = row * cellSize;
    drawRect(x, y, cellSize, cellSize, c1, c2, c3);
}

void fillRect(char x, char y, char w, char h, char c1, char c2, char c3) {
    setAddrWindow(x, y, x+w-1, y+h-1);
    ST7735_writeCommand(0x2C);

    
    for (int i = 0; i < w * h; i++) {
        ST7735_writeData(c1);
        ST7735_writeData(c2);
        ST7735_writeData(c3);
    }
}

int score;
void drawStartScreen() {
    ST7735_blackScreen();
}

void drawGameOver() {
    ST7735_blackScreen();
    drawCenteredNumber(score, 4, 4, cellSize, 255, 255, 255);
}

void clearCell(int row, int col) {
    int x = col * cellSize;
    int y = row * cellSize;
    fillRect(x+1, y+1, cellSize-2, cellSize-2, 0, 0, 0);
    drawRect(x, y, cellSize, cellSize, 255, 255, 255);
}
//10 Func/stop
//11 power
//12 vol+
//13 left
//14 pause
//15 right
//16 vol-
char getNum(long int val) {
    long int code[17] = {16738455, 16724175, 16718055, 16743045, 16716015, 16726215, 16734885, 16728765, 16730805, 16732845, 16769565, 16753245, 16736925, 16720605, 16712445, 16761405, 16754775};
    char num;
    for(int i = 0; i < 17; i++) {
        if(val == code[i]) {
            num = i;
        }
    }

    return num;

}

#define NUM_TASKS 7 //TODO: Change to the number of tasks being used

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;


//TODO: Define Periods for each task
const unsigned long TASKS1 = 100;
const unsigned long TASKS2 = 100;
const unsigned long TASKS3 = 100;
const unsigned long TASKS4 = 100;
const unsigned long TASKS5 = 100;
const unsigned long TASKS6 = 100;
const unsigned long TASKS7 = 100;

// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long GCD_PERIOD = 100;//TODO:Set the GCD Period
task tasks[NUM_TASKS]; // declared task array with 5 tasks
void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}


//Global Variables
decode_results results;
bool game_on = false;
bool startScreen = false;
bool game_over = false;
bool correctness = false;

long int currVal;
int cnt;
int col = 1;
int row = 1;

//TODO: Create your tick functions for each task
enum game_on_task {start, gameOn, gameover};
int TickFct_gameOn(int gt_state) {

    switch(gt_state) {
        case start:
            if(currVal == 11) {
                drawSudokuBoard();
                gt_state = gameOn;
            }
            else {
                gt_state = start;
            }
            break;
        case gameOn:
            if(correctness) {
                correctness = false;
                ST7735_blackScreen();
                drawGameOver();
                gt_state = gameover;
            }
            else if(currVal == 10) {
                ST7735_blackScreen();
                drawStartScreen();
                gt_state = start;
            }
            else {
                gt_state = gameOn;
            }
            break;
        case gameover:
            if(currVal == 10) {
                drawGameOver();
                gt_state = start;
            }
            else {
                gt_state = gameover;
            }
            break;
        default:
            gt_state = start;
            break;
    }

    switch(gt_state) {
        case start:
            startScreen = true;
            game_on = false;
            game_over = false;
            break;
        case gameOn:
            startScreen = false;
            game_on = true;
            game_over = false;
            break;
        case gameover:
            startScreen = false;
            game_on = false;
            game_over = true;
            break;
        default:
            break;
    }

    return gt_state;
}

//Receive signal every 100ms
enum IR_remote_task {wait};
int TickFct_IRremote(int it_state) {
    switch(it_state) {
        case wait:
            break;
        default:
            it_state = wait;
            break;
    }

    switch(it_state) {
        case wait:
            if (IRdecode(&results)) {
                //serial_println(getNum(results.value));
                currVal = getNum(results.value);
                IRresume();
            }
            break;
        default:
            break;
    }

    return it_state;
}

enum cursor_task {OFF, ON};
int TickFct_cursor(int et_state) {

    //10 Func/stop
    //11 power
    //12 vol+
    //13 left
    //14 pause
    //15 right
    //16 vol-
    switch(et_state) {
        case OFF:
            if(game_on) {
                highlightCell(row, col, 255, 255, 255);
                et_state = ON;
            }
            else {
                et_state = OFF;
            }
            break;
        case ON:
            if(!game_on) {
                highlightCell(row, col, 255, 0, 0);
                row = 1;
                col = 1;
                et_state = OFF;
            }
            else if(currVal == 12) {
                highlightCell(row, col, 0, 0, 255);
                row -= 1;
                if(row < 1) {
                    row = 9;
                }
                highlightCell(row, col, 255, 255, 255);
                currVal = -1;
                et_state = ON;
            }
            else if(currVal == 16) {
                highlightCell(row, col, 0, 0, 255);
                row += 1;
                if(row > 9) {
                    row = 1;
                }
                highlightCell(row, col, 255, 255, 255);
                currVal = -1;
                et_state = ON;

            }
            else if(currVal == 15) {
                highlightCell(row, col, 0, 0, 255);
                col += 1;
                if(col > 9) {
                    col = 1;
                }
                highlightCell(row, col, 255, 255, 255);
                currVal = -1;
                et_state = ON;
            }
            else if(currVal == 13) {
                highlightCell(row, col, 0, 0, 255);
                col -= 1;
                if(col < 1) {
                    col = 9;
                }
                highlightCell(row, col, 255, 255, 255);
                currVal = -1;
                et_state = ON;
            }
            else {
                et_state = ON;
            }
            break;
        default:
            et_state = OFF;
    }

    switch(et_state) {
        case OFF:
            break;
        case ON:
            break;
        default:
            break;
    }

    return et_state;
}

enum write_task {close, open};
int TickFct_write(int wt_state) {
    static long int tempVal = 0;

    switch(wt_state) {
        case close:
            if(game_on) {
                wt_state = open;
            }
            else {
                wt_state = close;
            }
            break;
        case open:
            if(!game_on) {
                wt_state = close;
            }
            else if(sudokuBoard[row-1][col-1] == 0 && currVal >= 0 && currVal <= 9) {
                userAnswer[row-1][col-1] = currVal;
                drawCenteredNumber(tempVal, row-1, col-1, cellSize, 0, 0, 0);
                tempVal = currVal;
                drawCenteredNumber(currVal, row-1, col-1, cellSize, 0, 255, 0);
                currVal = -1;
                wt_state = open;
                // for(int i = 0; i < 9; i++) {
                //     for(int j = 0; j < 9; j++) {
                //         serial_println(userAnswer[i][j]);
                //         if(j == 8) {
                //             serial_println("\n");
                //         }
                //     }
                // }
            }
            else if(sudokuBoard[row-1][col-1] == 0 && currVal == 14) {
                userAnswer[row-1][col-1] = 0;
                clearCell(row, col);
                currVal = -1;
                wt_state = open;
                // for(int i = 0; i < 9; i++) {
                //     for(int j = 0; j < 9; j++) {
                //         serial_println(userAnswer[i][j]);
                //         if(j == 8) {
                //             serial_println("\n");
                //         }
                //     }
                // }
            }
            else {
                wt_state = open;
            }
            break;
        default:
            break;
    }

    switch(wt_state) {
        case close:
            for(int i = 0; i < 9; i++) {
                for(int j = 0; j < 9; j++) {
                    userAnswer[i][j] = 0;
                }
            }
            break;
        case open:
            break;
        default:
            break;
    }

    return wt_state;
}

enum checkCorrectness_task {close1, check};
int TickFct_correct(int ct_state) {
    static int cnt = 0;
    switch(ct_state) {
        case close1:
            if(game_on) {
                cnt = 0;
                ct_state = check;
            }
            else {
                ct_state = close1;
            }
            break;
        case check:
            if(!game_on) {
                ct_state = close1;
            }
            else {
                ct_state = check;
            }
            break;
        default:
            ct_state = close1;
            break;
    }

    switch(ct_state) {
        case close1:
            break;
        case check:
            for(int i = 0; i < 9; i++) {
                for(int j = 0; j < 9; j++) {
                    if(correctAnswer[i][j] == userAnswer[i][j]) {
                        cnt += 1;
                    }
                }
            }
            if(cnt == 81) {
                correctness = true;
            }
            else {
                cnt = 0;
            }
            break;
        default:
            break;
    }

    return ct_state;
}

enum music_task {OFF1, OP_TONE1, OP_TONE2, OP_TONE3, OP_TONE4, OP_TONE5, END_TONE1, END_TONE2, END_TONE3, END_TONE4, END_TONE5};
int TickFct_Music(int mt_state) {
    static int i;
    static int j;

    switch(mt_state) {
        case OFF1:
            if (startScreen) {
                i = 0;
                j = 0;
                mt_state = OP_TONE1;
            } else if (game_over) {
                i = 0;
                j = 0;
                mt_state = END_TONE1;
            }
            break;
        case OP_TONE1:
            if(startScreen && i >= 2) {
                i = 0;
                mt_state = OP_TONE2;
            } else if(!startScreen) {
                mt_state = OFF1;
            }
            break;
        case OP_TONE2:
            if(startScreen && i >= 2) {
                i = 0;
                mt_state = OP_TONE3;
            } else if(!startScreen) {
                mt_state = OFF1;
            }
            break;
        case OP_TONE3:
            if(startScreen && i >= 2) {
                i = 0;
                mt_state = OP_TONE4;
            } else if(!startScreen) {
                mt_state = OFF1;
            }
            break;
        case OP_TONE4:
            if(startScreen && i >= 2) {
                i = 0;
                mt_state = OP_TONE5;
            } else if(!startScreen) {
                mt_state = OFF1;
            }
            break;
        case OP_TONE5:
            if(startScreen && i >= 2) {
                i = 0;
                j++;
                mt_state = OFF1;
            } else if(!startScreen) {
                mt_state = OFF1;
            }
            break;
        case END_TONE1:
            if(game_over && i >= 2) {
                i = 0;
                mt_state = END_TONE2;
            } else if(!game_over) {
                mt_state = OFF1;
            }
            break;
        case END_TONE2:
            if(game_over && i >= 2) {
                i = 0;
                mt_state = END_TONE3;
            } else if(!game_over) {
                mt_state = OFF1;
            }
            break;
        case END_TONE3:
            if(game_over && i >= 2) {
                i = 0;
                mt_state = END_TONE4;
            } else if(!game_over) {
                mt_state = OFF1;
            }
            break;
        case END_TONE4:
            if(game_over && i >= 2) {
                i = 0;
                mt_state = END_TONE5;
            } else if(!game_over) {
                mt_state = OFF1;
            }
            break;
        case END_TONE5:
            if(game_over && i >= 2) {
                i = 0;
                j++;
                mt_state = OFF1;
            } else if(!game_over) {
                mt_state = OFF1;
            }
            break;
        default:
            mt_state = OFF1;
            break;
    }

    switch(mt_state) {
        case OFF1:
            OCR1A = 0;
            break;
        case OP_TONE1:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 3000;  // Set TOP value for frequency
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        case OP_TONE2:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 4000;  // Set TOP value for frequency
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        case OP_TONE3:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 5000;  // Set TOP value for frequency
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        case OP_TONE4:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 4000;  // Set TOP value for frequency
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        case OP_TONE5:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 3000;  // Set TOP value for frequency
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        case END_TONE1:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 2000;  // Set TOP value for frequency (higher pitch)
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        case END_TONE2:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 2500;  // Set TOP value for frequency
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        case END_TONE3:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 3000;  // Set TOP value for frequency
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        case END_TONE4:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 3500;  // Set TOP value for frequency
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        case END_TONE5:
            i++;
            OCR1A = ICR1 / 2;  // 50% duty cycle
            ICR1 = 4000;  // Set TOP value for frequency (lower pitch)
            TCCR1B = (TCCR1B & 0xF8) | 0x03;  // Prescaler 64
            break;
        default:
            break;
    }
    return mt_state;
}

enum timer_task {OFF2, counting, displayScore};
int TickFct_timer(int tt_state) {
    static char i;
    switch(tt_state) {
        case OFF2:
            if(game_on) {
                i = 0;
                cnt = 0;
                tt_state = counting;
            }
            else {
                tt_state = OFF2;
            }
            break;
        case counting:
            if(game_over) {
                tt_state = displayScore;
                //drawGameOver();
            }
            else if(startScreen) {
                tt_state = OFF2;
            }
            else {
                tt_state = counting;
            }
        case displayScore:
            if(!game_over) {
                cnt = 0;
                tt_state = OFF2;
            }
            else {
                tt_state = displayScore;
            }
            break;
        default:
            break;
    }

    switch(tt_state) {
        case OFF2:
            break;
        case counting:
            if(i >= 10) {
                cnt++;
                i = 0;
            }
            if(cnt <= 500) {
                score = 9;
            }
            else if(cnt > 500 && cnt <= 1000) {
                score = 7;
            }
            else if(cnt > 1000 && cnt <= 1500) {
                score = 5;
            }
            else if(cnt > 1500 && cnt <= 2000) {
                score = 3;
            }
            else {
                score = 0;
            }
        case displayScore:
            break;
        default:
            break;
    }

    return tt_state;
}

int main(void) {
    //TODO: initialize all your inputs and ouputs
    //HardwareReset();
    ADC_init();   // initializes ADC
    serial_init(9600);
    SPI_INIT();
    
    DDRD = 0b11011111; PORTD = 0b00100000;
    DDRB = 0b11101101; PORTB = 0b00010010;
    DDRC = 0b00000000; PORTC = 0b11111111;


    ST7735_init();
    IRinit(&PORTD, &PIND, 5);
    //TODO: Initialize the buzzer timer/pwm(timer0)

    // TCCR0A |= (1 << COM0A1);
    // TCCR0A |= (1 << WGM01) | (1 << WGM00);

    TCCR1A |= (1 << COM1A1);
    TCCR1A |= (1 << WGM11);
    TCCR1B |= (1 << WGM12) | (1 << WGM13);

    //ICR1 = 39999; //20 ms pwm period



    DDRB |= (1 << PB1);

    
    // TCCR0B = (TCCR0B & 0xF8) | 0x02;    //set prescaler to 8
    // TCCR0B = (TCCR0B & 0xF8) | 0x03;    //set prescaler to 64
    // TCCR0B = (TCCR0B & 0xF8) | 0x04;    //set prescaler to 256
    // TCCR0B = (TCCR0B & 0xF8) | 0x05;    //set prescaler to 1024
    //TODO: Initialize the servo timer/pwm(timer1)


    //TODO: Initialize tasks here
    // e.g. 
    // tasks[0].period = TASKS1;
    // tasks[0].state = OFF;
    // tasks[0].elapsedTime = tasks[0].period;
    // tasks[0].TickFct = TickFct_opMusic;

    tasks[0].period = TASKS1;
    tasks[0].state = start;
    tasks[0].elapsedTime = tasks[0].period;
    tasks[0].TickFct = TickFct_gameOn;

    tasks[1].period = TASKS2;
    tasks[1].state = wait;
    tasks[1].elapsedTime = tasks[1].period;
    tasks[1].TickFct = TickFct_IRremote;

    tasks[2].period = TASKS3;
    tasks[2].state = OFF;
    tasks[2].elapsedTime = tasks[2].period;
    tasks[2].TickFct = TickFct_cursor;

    tasks[3].period = TASKS4;
    tasks[3].state = close;
    tasks[3].elapsedTime = tasks[3].period;
    tasks[3].TickFct = TickFct_write;

    tasks[4].period = TASKS5;
    tasks[4].state = close1;
    tasks[4].elapsedTime = tasks[4].period;
    tasks[4].TickFct = TickFct_correct;

    tasks[5].period = TASKS6;
    tasks[5].state = OFF1;
    tasks[5].elapsedTime = tasks[5].period;
    tasks[5].TickFct = TickFct_Music;

    tasks[6].period = TASKS7;
    tasks[6].state = OFF2;
    tasks[6].elapsedTime = tasks[6].period;
    tasks[6].TickFct = TickFct_timer;


    TimerSet(GCD_PERIOD);
    TimerOn();

    //drawSudokuBoard();
    //drawUI();

    // ST7735_blackScreen();
    // drawSudokuBoard();
    // drawCenteredNumber(0, 3, 1, cellSize, 255, 255, 255);
    
    //drawSudokuBoard();
    //highlightCell(1, 4, 255, 255, 255);



    //decode_results results;

    while(1) {
        // if (IRdecode(&results)) {
        //     serial_println(getNum(results.value));
        //     //serial_println(results.value);
        //     IRresume();
        // } else {
        //     serial_println(-1);
        // }
        // _delay_ms(500);
    }

    return 0;
}