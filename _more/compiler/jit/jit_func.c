int jit_func() {
    int a = 0, b = 0;
    /* PC=0 */
    a = 1;
    /* PC=1 */
    b = 2;
    /* PC=2 */
    a = a * b; b = 0;
    /* PC=3 */
    b = 3;
    /* PC=4 */
    a = a + b; b = 0;
    /* PC=5 */
    return a;
}
