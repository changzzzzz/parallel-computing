#include <stdio.h>

int main(){
    int size =4;
    int recv_data[4]={0,-1,-2,-3};
    int flag= 1;
    int K = 10;
    for (int i = 8; i < size; ++i) {
        if (recv_data[i] >= 0 && recv_data[i] < K- 2) {
            printf("%d\n",recv_data[i]);
            flag = 0;
        }else{
            printf("%d\n",recv_data[i]);
        }
    }
    //flag should be
    if(flag){
        printf("yes\n");
    }else{
        printf("no\n");
    }
    return 0;
}
