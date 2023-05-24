
#include<stdio.h>

int main(int argc, char *argv[]){

    char fileNameOut[200];

    int n;

    if(argc == 2){
        
        sprintf(fileNameOut,"%s.txt",argv[1]);

        printf("Convirtiendo archivo : %s\n\n",argv[1]);
        printf("Salida : %s",fileNameOut);

        FILE * file0;
        FILE * file1;
        file0 = fopen(argv[1],"r");
        file1 = fopen(fileNameOut,"w+");

        fseek(file0, 0, SEEK_END);
        // fread(&n_saved,sizeof(n_saved),1,file0);
        int sz = ftell(file0);

        printf("\nsize : %d\n",sz);

        
        fseek(file0, 0L, SEEK_SET);
        for(int i = 0 ; i < sz/sizeof(n); i++){
            fread(&n,sizeof(n),1,file0);
            fprintf(file1,"%d",n);
            fprintf(file1,"\n");
            if(i<=100)printf("%d\t",n);
        }
       
        printf("\n");

        fclose(file0);
        fclose(file1);


    }

    return 0;
}