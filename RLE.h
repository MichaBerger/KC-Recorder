#ifndef RLE_H
#define RLE_H

// declaration of external vars
extern volatile uint8_t pending;	// pending flag for Key handling
extern volatile uint8_t breakFlag;	// flag for abandoning record or play

// public function headers
void RLErecord(void);
void RLEreplay(void);

#endif // RLE_H
