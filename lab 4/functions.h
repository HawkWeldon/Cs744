#ifndef FUNCTIONS_H
#define FUNCTIONS_H
    typedef struct Route
        {
            char *url;
            int id;
        } 
    Route;

    #define NUM_FUNCS 7

    enum ID
    {
        ROOT,
        SQUARE,
        CUBE,
        HELLOWORLD,
        PINGPONG,
        PRIME,
        FIB
    };

    static const Route routes[] = {
                                    {"/", ROOT},
                                    {"/square", SQUARE},
                                    {"/cube", CUBE},
                                    {"/helloworld", HELLOWORLD},
                                    {"/pingpong", PINGPONG},
                                    {"/prime", PRIME},
                                    {"/fib", FIB}               };

#endif