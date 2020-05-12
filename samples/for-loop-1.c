

int x_arr[100];
int y_arr[100];

int res[100];
int *res_ptr;

int *x_ptr;
int *y_ptr;

void access() {
    int x = *x_ptr;
    int y = *y_ptr;
    *res_ptr = x + y;
    res_ptr++;
}


int main() {
    int i;
    x_ptr = &x_arr[0];
    y_ptr = &y_arr[0];
    res_ptr = &res[0];

    for (i=0; i<100; i++) {
        access();
        x_ptr++;
        y_ptr++;
    }
}

// (x_ptr = 8 * i + x_arr[0]) && (Y_ptr = 4 * i + y_arr[0])