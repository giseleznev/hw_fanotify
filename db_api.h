#include <sqlite3.h>
#include <string>
#include <unordered_map> //для хэша
#include <unistd.h>
#include <limits>
#include <limits>
#include <iostream>

sqlite3 *db;
int ModificationCounter;
std::string MaxMods("2");
int rc;

char *pError = NULL;
int fHasResult = 0;

int TRIVIAL_Callback(void *NotUsed, int argc, char **argv, char **azColName) {
    *(int*)NotUsed = 1;
    return 0;
}

int TRACE_Callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   return 0;
}

std::string Hash_Binary(int pid) {
    char buf[1024];
    int len;
    std::string binary_symlink = "/proc/"+ std::to_string(pid) + "/exe";
    if ((len = readlink(binary_symlink.c_str(), buf, sizeof(buf)-1)) != -1) buf[len] = '\0';
    if (len == 0 || len == -1) return "";

    FILE* fp = fopen(buf, "r");
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char *fdata = new char[sz];
    int sz_read = fread(fdata, 1, sz, fp);
    fclose(fp);

    std::string fdata_s(fdata); 
    std::size_t h1 = std::hash<std::string>{}(fdata_s);

    delete[] fdata;

    return std::to_string(h1);
}

char* Read_File_To_string(std::string filepath) {
    FILE* fp = fopen(filepath.c_str(), "r");
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char *fdata = new char[sz];
    int sz_read = fread(fdata, 1, sz, fp);
    printf("read file: %s\n", fdata);
    fclose(fp);
    return fdata; 
}


int Put_To_Blacklist(int pid) {
    std::string pid_s = std::to_string(pid);
    std::string check_black = "INSERT INTO BLACKLIST VALUES ('"+ Hash_Binary(pid) +"');";
    rc = sqlite3_exec(db, check_black.c_str(), TRIVIAL_Callback, &fHasResult, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "blacklist successfully\n");
    }
    std::string select3 = "SELECT * FROM BLACKLIST;";
    rc = sqlite3_exec(db, select3.c_str(), TRACE_Callback, &fHasResult, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "blacklist select successfully\n");
    }
    return 0;
}

int Restore_File(char* path, char* source_data) {
    printf("restoring: %s , %s\n", path, source_data);
    return 0;
}

int RESTORE_Callback(void *NotUsed, int argc, char **argv, char **azColName) {
    Restore_File(argv[0], argv[1]);
    return 0;
}

// int DELETE_Callback(void *NotUsed, int argc, char **argv, char **azColName) {
//     for (int i = 0; i < argc ; i += 2) {
//         std::string file_path_to_delete(argv[i]);
//         std::string delete_mod = "DELETE FROM MODIFY WHERE PATH='" + file_path_to_delete + "';";
//         rc = sqlite3_exec(db, delete_mod.c_str(), TRIVIAL_Callback, &fHasResult, &pError);
//         if( rc != SQLITE_OK ){
//             fprintf(stderr, "SQL error: %s\n", pError);
//             sqlite3_free(pError);
//         } else {
//             fprintf(stdout, "delete from modify successfully\n");
//         }
//     }
//     return 0;
// }

int Restore_Files(int pid) {
    std::string select3 = "SELECT * FROM FILES;";
    rc = sqlite3_exec(db, select3.c_str(), TRACE_Callback, &fHasResult, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "select successfully\n");
    }
    std::string pid_s = std::to_string(pid);
    std::string check_put_black_list = "SELECT PATH, SOURCE FROM FILES WHERE PATH in (SELECT PATH FROM MODIFY WHERE PID ='" + pid_s + "');";
    rc = sqlite3_exec(db, check_put_black_list.c_str(), RESTORE_Callback, &fHasResult, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "restored successfully\n");
    }
    // std::string delete_proc = "DELETE FROM MODIFY WHERE PID='" + pid_s + "';";
    // rc = sqlite3_exec(db, delete_proc.c_str(), TRIVIAL_Callback, &fHasResult, &pError);
    // if( rc != SQLITE_OK ){
    //     fprintf(stderr, "SQL error: %s\n", pError);
    //     sqlite3_free(pError);
    // } else {
    //     fprintf(stdout, "delete from modify successfully\n");
    // }
    // std::string delete_files = "DELETE FROM FILES WHERE PATH='" + file_path_to_delete + "';";
    // rc = sqlite3_exec(db, delete_files.c_str(), TRIVIAL_Callback, &fHasResult, &pError);
    // if( rc != SQLITE_OK ){
    //     fprintf(stderr, "SQL error: %s\n", pError);
    //     sqlite3_free(pError);
    // } else {
    //     fprintf(stdout, "delete from files successfully\n");
    // }
    return 0;
}

int InitDB(){
    std::string sql;
    rc = sqlite3_open(":memory:", &db);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    /* Create SQL statement */
    sql = "CREATE TABLE FILES("  \
        "PATH           TEXT PRIMARY KEY NOT NULL," \
        "TIME           TEXT," \
        "SOURCE         TEXT);";
    rc = sqlite3_exec(db, sql.c_str(), TRIVIAL_Callback, 0, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "Table added successfully\n");
    }
    /* Create SQL statement */
    sql = "CREATE TABLE PROCESSES("  \
        "PID        INT PRIMARY KEY NOT NULL," \
        "NUM_MOD    INT);";
    rc = sqlite3_exec(db, sql.c_str(), TRIVIAL_Callback, 0, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "Table added successfully\n");
    }
    /* Create SQL statement */
    sql = "CREATE TABLE MODIFY("  \
        "ID                     INT PRIMARY KEY NOT NULL," \
        "PID                    INT," \
        "PATH                   TEXT," \
        "FOREIGN KEY(PID)       REFERENCES PROCESSES(PID)," \
        "FOREIGN KEY(PATH) REFERENCES FILES(PATH));";
    rc = sqlite3_exec(db, sql.c_str(), TRIVIAL_Callback, 0, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "Table added successfully\n");
    }
    /* Create SQL statement */
    sql = "CREATE TABLE BLACKLIST("  \
        "HASH         TEXT PRIMARY KEY NOT NULL);";
    rc = sqlite3_exec(db, sql.c_str(), TRIVIAL_Callback, 0, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "Table added successfully\n");
    }
    return 0;
}

int On_Modify(int pid, std::string filepath) {
    std::cout << "in Modify func" << std::endl;
    std::string ModCounter = std::to_string(ModificationCounter);
    std::string pid_s = std::to_string(pid);
    std::string file_string = Read_File_To_string(filepath);
    // std::string hash_s = "abc";
    // Hash_Binary(pid);
    // if (hash_s == "") {
    //     std::cout << "hash is null" << std::endl;
    //     return 0;
    // } // пропускаем так как какой-то системный процесс ?

    // std::cout << "SELECT" << std::endl;
    fHasResult = 0;
    std::string check_black = "SELECT 1 FROM BLACKLIST WHERE HASH='" + Hash_Binary(pid) + "';";
    rc = sqlite3_exec(db, check_black.c_str(), TRIVIAL_Callback, &fHasResult, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "select from blacklist successfully\n");
    }
    if(fHasResult) {
        std::cout << "Detected earlier" << std::endl;
        return 1;
    }// вирус найденный ранее

    // std::string check_white = "SELECT 1 FROM WHITELIST WHERE HASH='" + hash_s + "';";
    // rc = sqlite3_exec(db, check_black.c_str(), TRIVIAL_Callback, &fHasResult, &pError);
    // if( rc != SQLITE_OK ){
    //     fprintf(stderr, "SQL error: %s\n", pError);
    //     sqlite3_free(pError);
    // } else {
    //     fprintf(stdout, "SELECT 1 FROM WHITELIST successfully\n");
    // }
    // if(fHasResult) {
    //     return 0;
    // }// пропускаем без вопросов

    // std::string check_pid = "SELECT *,
    //                         CASE \
    //                             WHEN EXISTS (SELECT NUM_MOD as NUM_MODS FROM PROCESSES WHERE PID='" + pid_s + "')  THEN UPDATE PROCESSES SET NUM_MOD=(NUM_MODS + 1) WHERE PID='" + pid_s + "' \
    //                             ELSE INSERT PROCESSES VALUES ('"+ pid_s + "', 0) \
    //                         END;";
    std::string check_pid = "INSERT OR REPLACE INTO PROCESSES   \
                            VALUES ('" + pid_s + "',    \
                            COALESCE(   \
                                (SELECT NUM_MOD FROM PROCESSES \
                                WHERE PID='" + pid_s + "'), 0) + 1);";
    rc = sqlite3_exec(db, check_pid.c_str(), TRIVIAL_Callback, &fHasResult, &pError); // добавить инфу о процессе
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "PID added to DB successfully\n");
    }
        std::string select2 = "SELECT * FROM PROCESSES;";
    rc = sqlite3_exec(db, select2.c_str(), TRACE_Callback, &fHasResult, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "select successfully, %d\n",fHasResult);
    }
    fHasResult = 0;
    std::string check_put_black_list = "SELECT * FROM PROCESSES WHERE PID=" + pid_s + " AND NUM_MOD>"+ MaxMods +";";
    rc = sqlite3_exec(db, check_put_black_list.c_str(), TRIVIAL_Callback, &fHasResult, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "NumMod readed successfully, %d\n", fHasResult);
    }
    if(fHasResult) {
        Put_To_Blacklist(pid);
        Restore_Files(pid);
        return 1;
    }// обнаружен вирус

    // std::string check_file = "CASE \
    //                             WHEN EXISTS (SELECT 1 FROM FILES WHERE HASH_PATH_FILE='" + hash_p + "') \
    //                                 THEN UPDATE FILES SET TIME=GETDATE() WHERE HASH_PATH_FILE='" + hash_p + "' \
    //                             ELSE \
    //                                 INSERT FILES VALUES ('"+ hash_p + "'," + filepath + ", GETDATE()) \
    //                         END;";
    std::string check_file = "INSERT OR IGNORE INTO FILES VALUES ('" + filepath + "', (SELECT DATE()), '" + file_string + "'); \
                        UPDATE FILES SET TIME=(SELECT DATE()) WHERE PATH='" + filepath + "';";
    rc = sqlite3_exec(db, check_file.c_str(), TRIVIAL_Callback, &fHasResult, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "Files updated successfully\n");
    }

    ModificationCounter++;
    std::string insert_modify = "INSERT INTO MODIFY VALUES ('"+ ModCounter+ "','"+ pid_s + "','"+ filepath + "');";
    rc = sqlite3_exec(db, insert_modify.c_str(), TRIVIAL_Callback, &fHasResult, &pError);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", pError);
        sqlite3_free(pError);
    } else {
        fprintf(stdout, "Modify updated successfully\n");
    }

    // std::string select1 = "SELECT * FROM FILES;";
    // rc = sqlite3_exec(db, select1.c_str(), TRACE_Callback, &fHasResult, &pError);
    // if( rc != SQLITE_OK ){
    //     fprintf(stderr, "SQL error: %s\n", pError);
    //     sqlite3_free(pError);
    // } else {
    //     fprintf(stdout, "select successfully\n");
    // }
    // std::string select2 = "SELECT * FROM PROCESSES;";
    // rc = sqlite3_exec(db, select2.c_str(), TRACE_Callback, &fHasResult, &pError);
    // if( rc != SQLITE_OK ){
    //     fprintf(stderr, "SQL error: %s\n", pError);
    //     sqlite3_free(pError);
    // } else {
    //     fprintf(stdout, "select successfully\n");
    // }
    // std::string select3 = "SELECT * FROM MODIFY;";
    // rc = sqlite3_exec(db, select3.c_str(), TRACE_Callback, &fHasResult, &pError);
    // if( rc != SQLITE_OK ){
    //     fprintf(stderr, "SQL error: %s\n", pError);
    //     sqlite3_free(pError);
    // } else {
    //     fprintf(stdout, "select successfully\n");
    // }
    return 0; // пропустить
}
