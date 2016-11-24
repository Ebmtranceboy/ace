#ifndef ACE_H
#define ACE_H
typedef short bool;

typedef union _primitive primitive;
typedef struct _atom atom;

typedef struct _stack{
        atom* head;
        struct _stack* rest;
}stack;

typedef struct _application{
        atom* f;
        atom* x;
}application;

typedef struct _function{
        atom* (*f)(stack*);
        int arity;
}function;

typedef struct _recursive{
        atom* (*rec)(stack*);
        atom* arg;
        atom* _;
        atom* iter;
        stack* cache;
}recursive;

union _primitive{
        recursive r;
        function f;
        application a;
        int n;
        bool b;
        char c;
        float x;
};



#define        CHAR     0
#define        INT      1
#define        BOOL     2
#define        FLOAT    3
#define        CMBNTR_S 4
#define        CMBNTR_K 5
#define        CMBNTR_I 6
#define        CMBNTR_B 7
#define        CMBNTR_C 8
#define        APP      9
#define        FUN      10
#define        LIST     11
#define        STRING   12
#define        REC      13
#define        ITEM     14


struct _atom{
        primitive p;
        int k;
};

atom *S, *K, *I, *B, *C;
atom* emptyString, *nil;
atom* true, *false;

atom *undefined, *error, *as;
atom *tail, *length, *cons, *isEmpty, *head;
atom *plus, *times, *dividedBy, *power, *quot, *rem, *subtract, *negate;
atom *euclid, *gcd;
atom *foldr, *foldl, *scanl, *map, *take, *drop, *takeWhile, *dropWhile;
atom *repeat, *cycle, *insert, *permutations;
atom *ifte, *equal, *equiv, *not, *and;
atom *lessThan, *greaterThan, *lte, *gte, *min, *max;
atom *factorial, *fibonacci;
atom *filter, *elem, *nth, *transpose;
atom *uncurry, *fix, *sortBy, *sort, *group, *groupBy;
atom *randint, *module;
atom *select_ace, *powerSet, *cartesianProduct, *range;

atom *append, *reverse, *init, *last, *concat;
atom *minus, *even, *odd, *fromEnum;
atom *zip, *zipWith, *nub, *on;

atom *pair, *fst, *snd;
atom *or, *predicate, *any, *all;
atom *scanr;
atom* dummy;

atom *sow, *customs, *algo_euclid;

atom* apply(atom* , atom* );
atom* intc(int );
atom* floatc(float );
atom* apply2(atom* , atom* , atom* );
atom* apply3(atom* , atom* , atom* , atom* );
atom* parse(char*);
char* toString(atom*);
void println(atom*);
void print(atom*);
atom* new();
void declareFunction(atom **, int , atom* (*)(stack*));
void initialize();
void repl();
void stamp(atom *, atom*);
atom* fromString(const char*);
atom* getName(atom*);
bool areSameStrings(atom*,atom*);
void empty(stack*);
#endif

