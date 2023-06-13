# Command Line Tarot Card Deck
This CLI program creates a tarot card type, creates an array of tarot cards as a deck, 
and lets the user enter a number to do a draw of n cards. Default 3. 
Cards can be reversed or not. 

The deck list is too big to be compiled into internal flash memory. 
So it must be stored in Apps Asset folder on SD card and then retrieved 
at runtime. Use STORAGE_APP_ASSETS_PATH_PREFIX for file path to SD card.