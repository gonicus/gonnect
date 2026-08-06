#include <stdio.h>
#include <string.h>

#include <libstemmer.h>

int main(void)
{
    struct sb_stemmer * stemmer = sb_stemmer_new("german", "UTF_8");
    if (!stemmer) {
        fprintf(stderr, "sb_stemmer_new failed\n");
        return 1;
    }

    const char * word = "Verbindungen";
    const sb_symbol * stem = sb_stemmer_stem(stemmer, (const sb_symbol *)word, (int)strlen(word));
    if (!stem) {
        fprintf(stderr, "sb_stemmer_stem failed\n");
        sb_stemmer_delete(stemmer);
        return 1;
    }

    printf("%s -> %s\n", word, (const char *)stem);
    sb_stemmer_delete(stemmer);
    return 0;
}
