inline int rgb(double r, double g, double b) {
    return rgb((int)(r*255+.5),(int)(g*255+.5),(int)(b*255+.5));
}

/** Draws a line. */
void line(double x1, double y1, double x2, double y2, int c) {
    int d = (int)(1+max(abs(x1-x2),abs(y1-y2)));
    for (int i=0; i<=d; i++) {
        double x=(x1+(x2-x1)*i/d);
        double y=(y1+(y2-y1)*i/d);
        pix(x,y,c);
    }
}

/** Fills the screen with a nice rainbow spiral. */
void spirals() {
    int N = 6;
    int D = 10;
    int A[] = { 0x000001, 0x000101, 0x000100, 0x010100, 0x010000, 0x010001, };
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            int cx=x-SCREEN_WIDTH/2;
            int cy=y-SCREEN_HEIGHT/2;
            double d = hypot(cx,cy);
            double z = d + N*D*(atan2(cx,cy)/M_PI/2+.5);
            int n = (int)(fmod(z,N*D)/D);
            z = 2.5-fabs(fmod(z,D)-D/2)/2;
            d = d/320;
            if (d>1) d=1;
            z*=d;
            double q=z-1;
            if (z>1) z=1;
            if (z<0) z=0;
            if (q>1) q=1;
            if (q<0) q=0;
            int w = (int)(z*255+.5);
            int v = (int)(q*255+.5);
            pix(x,y,w*A[n]+v*A[(n+3)%N]);
        }
    }
}
