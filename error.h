
#ifndef ERROR_H
#define ERROR_H


void Warning (const char* Format, ...);

void Error (const char* Format, ...);

//void ErrorCode (int Code, const char* Format, ...) attribute((noreturn, format(printf,2,3)));
///* Print an error message and die with the given exit code */

//void Internal (const char* Format, ...) attribute((noreturn, format(printf,1,2)));
///* Print an internal error message and die */

//void SimExit (int Code);
///* Exit the simulation with an exit code */



/* End of error.h */

#endif
