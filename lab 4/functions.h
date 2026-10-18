typedef struct
{
    char *url;
    int id;
} 
Route;

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

Route routes[] = {
                    {"/", ROOT},
                    {"/square", SQUARE},
                    {"/cube", CUBE},
                    {"/helloworld", HELLOWORLD},
                    {"/pingpong", PINGPONG},
                    {"/prime", PRIME},
                    {"/fib", FIB}               };