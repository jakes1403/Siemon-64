#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Phoneme {
    int time;
    char mouth_type[4];
    struct Phoneme* next;
} Phoneme;

typedef struct Word {
    char word[20];
    Phoneme* phoneme;
    struct Word* next;
} Word;

Phoneme* newPhoneme(int time, char mouth_type[4]) {
    Phoneme* newPhoneme = (Phoneme*)malloc(sizeof(Phoneme));
    newPhoneme->time = time;
    strcpy(newPhoneme->mouth_type, mouth_type);
    newPhoneme->next = NULL;
    return newPhoneme;
}

Word* newWord(char word[20]) {
    Word* newWord = (Word*)malloc(sizeof(Word));
    strcpy(newWord->word, word);
    newWord->phoneme = NULL;
    newWord->next = NULL;
    return newWord;
}

void freeData(Word* head) {
    while (head) {
        Word* tempWord = head;
        Phoneme* phoneme = head->phoneme;
        while (phoneme) {
            Phoneme* tempPhoneme = phoneme;
            phoneme = phoneme->next;
            free(tempPhoneme);
        }
        head = head->next;
        free(tempWord);
    }
}

Word* parsePapagayoFile(const char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        debugf("Unable to open file. %s\n", file_path);
        return NULL;
    }

    Word* head = NULL;
    Word* lastWord = NULL;
    Phoneme* lastPhoneme = NULL;
    char buffer[100];

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (strstr(buffer, ".wav") != NULL || strstr(buffer, ".WAV") != NULL) {
            continue;
        }

        int time;
        char mouth_type[4], word[20];
        if (sscanf(buffer, "%s %d %*d %*d", word, &time) == 2) {
            Word* new_word = newWord(word);
            if (head == NULL) {
                head = new_word;
                lastWord = new_word;
            } else {
                lastWord->next = new_word;
                lastWord = new_word;
            }
            lastPhoneme = NULL;
        }
        else if (sscanf(buffer, "%d %s", &time, mouth_type) == 2) {
            Phoneme* new_phoneme = newPhoneme(time, mouth_type);
            if (lastWord->phoneme == NULL) {
                lastWord->phoneme = new_phoneme;
                lastPhoneme = new_phoneme;
            } else {
                lastPhoneme->next = new_phoneme;
                lastPhoneme = new_phoneme;
            }
        }
    }

    fclose(file);
    return head;
}

void printParsedData(Word* head) {
    Word* tempWord = head;
    while(tempWord != NULL) {
        debugf("Word: %s\n", tempWord->word);
        Phoneme* tempPhoneme = tempWord->phoneme;
        while(tempPhoneme != NULL) {
            debugf("\tTime: %d, Mouth Type: %s\n", tempPhoneme->time, tempPhoneme->mouth_type);
            tempPhoneme = tempPhoneme->next;
        }
        tempWord = tempWord->next;
    }
}