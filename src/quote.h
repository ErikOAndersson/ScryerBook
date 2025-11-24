#ifndef QUOTE_H
#define QUOTE_H

#include <Arduino.h>

// Api from api-ninjas.com
#define API_NINJAS_URL "https://api.api-ninjas.com/v1/"
#define API_NINJAS_KEY "oc9IJzYB5MIJToPwxULcEg==7Xsm6HptY9Pclqk8"

// Initialize quote system
void initQuote();

// Fetch a quote from API Ninjas
// Returns malloc'd char array containing the quote (CALLER MUST FREE!)
// Returns nullptr on error
char* GetNinjaQuote();

// Temporary for testing, remove this
const char tmp_quote[] = "I slept with faith and found a corpse in my arms on awakening I drank and danced all night with doubt and found her a virgin in the morning.";

#endif // QUOTE_H
