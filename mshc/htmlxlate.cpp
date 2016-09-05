
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

#include <string>

using namespace std;

static void chomp(char *s) {
    char *e = s + strlen(s) - 1;

    while (e > s && (*e == '\r' || *e == '\n')) *e-- = 0;
}

FILE *cat_fp = NULL;
FILE *ls_fp = NULL;

void copytranslate(const char *src,const char *dst) {
    xmlDocPtr doc;

    doc = xmlReadFile(src,NULL,XML_PARSE_RECOVER|XML_PARSE_NONET);
    if (doc == NULL) return;

    xmlSaveFileEnc(dst,doc,"UTF-8");
    xmlFreeDoc(doc);
}
                
void copyfile(const char *src,const char *dst) {
    unsigned char buf[4096];
    int src_fd,dst_fd;
    int rd,wd;

    src_fd = open(src,O_RDONLY);
    if (src_fd < 0) return;

    dst_fd = open(dst,O_WRONLY|O_CREAT|O_TRUNC,0644);
    if (dst_fd < 0) {
        close(src_fd);
        return;
    }

    while ((rd=read(src_fd,buf,sizeof(buf))) > 0) {
        if ((wd=write(dst_fd,buf,rd)) < rd)
            break;
    }

    close(dst_fd);
    close(src_fd);
}

int main() {
    struct stat st;
    string newpath;
    char line[1024];

    ls_fp = popen("find","r");
    if (ls_fp == NULL) return 1;

    cat_fp = fopen("catalog.txt","r");
    if (cat_fp == NULL) return 1;

    mkdir("HTML",0755);

    while (!feof(ls_fp) && !ferror(ls_fp)) {
        if (fgets(line,sizeof(line)-1,ls_fp) == NULL) break;
        chomp(line);

        if (!strncmp(line,"./FINAL/",8)) {
            /* must be .htm or .html */
            char *ext = strrchr(line,'.');
            if (ext == NULL) continue;

            newpath = "./HTML/";
            newpath += line+8;

            if (lstat(line,&st)) continue;

            if (S_ISREG(st.st_mode)) {
                if (!strcmp(ext,".htm") || !strcmp(ext,".html")) {
                    printf("Translate: %s\n",line);
                    printf("       to: %s\n",newpath.c_str());
                    copytranslate(line,newpath.c_str());
                }
                else {
                    printf("Copying: %s\n",line);
                    printf("     to: %s\n",newpath.c_str());
                    copyfile(line,newpath.c_str());
                }
            }
            else if (S_ISDIR(st.st_mode)) {
                mkdir(newpath.c_str(),0755);
            }
        }
    }

    fclose(cat_fp);
    fclose(ls_fp);
    return 0;
}

