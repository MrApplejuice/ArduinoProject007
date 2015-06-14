#ifndef PWM_MUSIC_H_
#define PWM_MUSIC_H_

// Sets which pin the library should use
void setMusicSoundPin(int i);

/*
 * Tries to play a multi tone wave file from the 
 * SD card using pulse width modulation.
 * 
 * Quite buggy, dows not work :-/
 */
bool playMusic(const char* filename);

#endif // PWM_MUSIC_H_

