
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

void GetHelpId(const char *path) {
    xmlNodePtr html=NULL,head=NULL,meta=NULL;
    xmlDocPtr doc;

    /* NTS: MSHC are generally XHTML */

    doc = xmlReadFile(path,NULL,XML_PARSE_RECOVER|XML_PARSE_NONET);
    if (doc == NULL) return;

    /* look for the HTML -> HEAD -> META entries */
    html = xmlDocGetRootElement(doc);
    while (html != NULL) {
        if (!xmlStrcmp(html->name,(const xmlChar*)"html"))
            break;

        html = html->next;
    }

    if (html != NULL) {
        head = html->children;
        while (head != NULL) {
            if (!xmlStrcmp(head->name,(const xmlChar*)"head"))
                break;

            head = head->next;
        }
    }

    if (head != NULL) {
        meta = head->children;
        while (meta != NULL) {
            if (!xmlStrcmp(meta->name,(const xmlChar*)"meta")) {
                string name,content;
                xmlChar *xp;

                xp = xmlGetNoNsProp(meta,(const xmlChar*)"name");
                if (xp != NULL) {
                    name = (char*)xp;
                    xmlFree(xp);
                }

                xp = xmlGetNoNsProp(meta,(const xmlChar*)"content");
                if (xp != NULL) {
                    content = (char*)xp;
                    xmlFree(xp);
                }

                if (name == "Microsoft.Help.Id") {
                    fprintf(cat_fp,"%s\n",path);
                    fprintf(cat_fp,"-id:%s\n",content.c_str());
                    fprintf(cat_fp,"\n");
                }
            }

            meta = meta->next;
        }
    }

    /* done */
    xmlFreeDoc(doc);
}

int main() {
    char line[1024];

    ls_fp = popen("find","r");
    if (ls_fp == NULL) return 1;

    cat_fp = fopen("catalog.txt","w");
    if (cat_fp == NULL) return 1;

    while (!feof(ls_fp) && !ferror(ls_fp)) {
        if (fgets(line,sizeof(line)-1,ls_fp) == NULL) break;
        chomp(line);

        /* must be .htm or .html */
        char *ext = strrchr(line,'.');
        if (ext == NULL) continue;

        if (!strcmp(ext,".htm") || !strcmp(ext,".html")) {
            printf("Reading %s\n",line);
            GetHelpId(line);
        }
    }

    fclose(cat_fp);
    fclose(ls_fp);
    return 0;
}

