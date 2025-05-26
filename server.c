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
#define MAX_MENU 10

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

MenuItem menu[MAX_MENU] = {
    {1, "ข้าวผัด", 50.0},
    {2, "ผัดไทย", 45.0},
    {3, "ต้มยำกุ้ง", 60.0},
    {4, "น้ำเปล่า", 10.0},
    {5, "ชาเย็น", 25.0},
    {6, "ข้าวไข่เจียว", 40.0},
    {7, "สุกี้", 70.0}
    
};

Table tables[MAX_TABLES];
void draw_tables(Table tables[], int total) {
    printf("╔════════╦═══════════════╦════════════╗\n");
    printf("║  โต๊ะ   ║   สถานะ       ║  ลูกค้า      ║\n");
    printf("╠════════╬═══════════════╬════════════╣\n");

    for (int i = 0; i < total; i++) {
        char* status;
        char* name;
        if(tables[i].is_occupied){
            status = "ไม่ว่าง";
            name = tables[i].customer_name;
        }
        else{
            status = "ว่าง";
            name = "ยังไม่มีลูกค้า";
        }

        printf("║  %2d   ║  %-12s ║  %-8s ║\n", tables[i].id, status, name);
    }

    printf("╚════════╩═══════════════╩════════════╝\n");
}

float calculate_total_price(char* order) {
    float total = 0.0;
    char temp[200];
    strcpy(temp, order); 
    char* token = strtok(temp, ","); //copy order ไปไว้ใน tempเพื่อให้ไม่ไปแก้ string ต้นฉบับ

    while (token != NULL) {
        int id = atoi(token); //แปลงจาก str เป็น int
        for (int i = 0; i < MAX_MENU; i++) {
            if (menu[i].id == id) {
                total += menu[i].price;
                break;
            }
        }
        token = strtok(NULL, ",");
    }

    return total;
}

void* handle_client(void* arg) {
    int clientSd = *(int*)arg; //รับ socket ที่ client เชื่อมเข้ามา
    int table_id;
    char buffer[1024]; //ตัวแปรชั่วคราวไว้เก็บข้อความที่รับจาก client
    char customer_name[50];

    free(arg); //คืนหน่วยความจำที่จองไว้ตอน malloc

    //รับชื่อ
    recv(clientSd, buffer, sizeof(buffer), 0); 
    strcpy(customer_name, buffer); //copy ชื่อจาก buffer ไปไว้ใน customer_name
    printf("ลูกค้าใหม่: %s\n", customer_name);

    while(1) {

        recv(clientSd, buffer, sizeof(buffer), 0);
        table_id = atoi(buffer);

        if (table_id < 1 || table_id > MAX_TABLES) {
            send(clientSd, "หมายเลขโต๊ะไม่ถูกต้อง กรุณาเลือกใหม่", 1024, 0);
            continue;  
        }

        if (tables[table_id - 1].is_occupied) {
            send(clientSd, "โต๊ะนี้ไม่ว่าง กรุณาเลือกโต๊ะอื่น", 1024, 0);
            continue;
        }

        else{
            tables[table_id - 1].is_occupied = 1;
            strcpy(tables[table_id - 1].customer_name, customer_name);
            strcpy(tables[table_id - 1].order, "");  // เคลียร์ order
            send(clientSd, "จองโต๊ะสำเร็จ", 1024, 0);
            printf("%s จองโต๊ะที่ %d\n", customer_name, table_id);
            draw_tables(tables, MAX_TABLES);
            break;  
        }


    }

    //รับรายการอาหาร วนรับเมนูหลายรายการ
    while (1) {
        memset(buffer, 0, sizeof(buffer)); //เคลียร์ buffer ให้ว่าง เพื่อไม่ให้ข้อความใหม่ซ้อนกับข้อความเก่า
        recv(clientSd, buffer, sizeof(buffer), 0);

        if (strcmp(buffer, "exit") == 0) {
            float total = calculate_total_price(tables[table_id - 1].order);
            char msg[128];
            sprintf(msg, "ยอดรวมทั้งหมด: %.2f บาท", total);
            send(clientSd, msg, strlen(msg) + 1, 0);

            // ปล่อยโต๊ะ
            tables[table_id - 1].is_occupied = 0;
            strcpy(tables[table_id - 1].customer_name, "");
            strcpy(tables[table_id - 1].order, "");
            printf("โต๊ะ %d ถูกปล่อยแล้ว\n", table_id);
            break;
        }

        // เพิ่มเมนูต่อท้าย (เช่น "1,4,2")
        if (strlen(tables[table_id - 1].order) > 0) {
            strcat(tables[table_id - 1].order, ",");
        }
        strcat(tables[table_id - 1].order, buffer);

        printf("โต๊ะ %d สั่งเมนู: %s\n", table_id, tables[table_id - 1].order);
        send(clientSd, "รับออเดอร์แล้ว", 1024, 0);
    }

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


