#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8000
#define MAX_CLIENTS 100

#define MAX_TABLES 20
#define TABLES_PER_ROW 4
#define MAX_MENU 5

typedef struct {
    int id;                     // เลขโต๊ะ 1-20
    int is_occupied;            // 0 = ว่าง, 1 = ไม่ว่าง 
    char customer_name[50];     // ชื่อลูกค้า
    char order[200];            // รายการอาหารที่ลูกค้าสั่ง
} Table;

typedef struct {
    int id;
    char name[50];
    float price;
} MenuItem;

Table tables[MAX_TABLES];
void draw_tables(Table tables[], int total) {
    int rows = (total + TABLES_PER_ROW - 1) / TABLES_PER_ROW; 
    int index = 0;
    
    printf("=========================================\n");
    for (int r = 0; r < rows; r++) {
        //ด้านบน
        for (int i = 0; i < TABLES_PER_ROW && index + i < total; i++) {
            printf("#####\t");
        }
        printf("\n");

        // แถวกลางกับ id
        for (int i = 0; i < TABLES_PER_ROW && index + i < total; i++) {
            printf("##%2d#\t", tables[index + i].id);
        }
        printf("\n");

        // ด้านล่าง
        for (int i = 0; i < TABLES_PER_ROW && index + i < total; i++) {
            printf("#####\t");
        }
        printf("\n");

        // status
        for (int i = 0; i < TABLES_PER_ROW && index + i < total; i++) {
            char* status;
            if(tables[index + i].is_occupied){
                status  = "ไม่ว่าง";
            }
            else {
                status = "ว่าง";
            }   
            printf("(%s)\t", status);
        }

        printf("\n\n");
        index += TABLES_PER_ROW;
    }
    printf("=========================================\n");
}

void* handle_client(void* arg) {
    int clientSd = *(int*)arg;
    char buffer[1024];

    recv(clientSd, buffer, sizeof(buffer), 0);
    printf("รับข้อความ: %s\n", buffer);

    char reply[] = "เซิร์ฟเวอร์ตอบกลับ\n";
    send(clientSd, reply, strlen(reply), 0);

    close(clientSd);
    return NULL;
}

int main() {
   
    int sd, newSd; //sd = socket หลัก , newSd = socket เฉพาะของ client ที่เชื่อมเข้ามา
    struct sockaddr_in servAddr, cliAddr; //servAddr = โครงสร้างที่เก็บข้อมูลของเซิฟ ip กับ port , cliAddr = ข้อมูลของ client ที่เชื่อมมา
    socklen_t cliLen = sizeof(cliAddr); //cliLen = ขนาดของโครงสร้าง cliAddr
    pthread_t tid; //ใช้สำหรับสร้าง thread

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&servAddr, 0, sizeof(servAddr)); //เคลียร์ข้อมูลใน servAddr ทั้งหมดให้เป็น 0
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT); //ระบุหมายเลขพอร์ตที่ server จะรอ
    servAddr.sin_addr.s_addr = INADDR_ANY; //รับการเชื่อมต่อทุก ip ที่เข้ามาเครื่องนี้ 

    bind(sd, (struct sockaddr*)&servAddr, sizeof(servAddr)); //ผูก socket sd กับ IP และ PORT ที่กำหนดไว้ใน servAddr
    listen(sd, 5);


    printf("Server กำลังรอ client บน port %d...\n", PORT);

    while (1) {
        newSd = accept(sd, (struct sockaddr*)&cliAddr, &cliLen); //รอ client 
        printf("Client ใหม่เชื่อมต่อ\n");// ถ้า client เข้ามาจะสร้าง socket ของตัวมันเอง

        
        int* pclient = malloc(sizeof(int)); //จองหน่วยความจำ เฉพาะตัวเอง
        *pclient = newSd;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid); 
    }

    close(sd);
    return 0;
}


