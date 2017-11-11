
/* run this at the top level containing the entire CHM collection (MSDN library 6.0a) */

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

    while (e >= s && (*e == '\r' || *e == '\n')) *e-- = 0;
}

FILE *cat_fp = NULL;
FILE *ls_fp = NULL;

void lookForAnchors(xmlNodePtr node,const char *path) {
    while (node != NULL) {
        if (node->name == NULL) {
        }
        else if (!strcasecmp((char*)node->name,"a")) {
            string name;

            {
                xmlChar *xp;
                xp = xmlGetNoNsProp(node,(const xmlChar*)"name");
                if (xp != NULL) {
                    /* MSDN 6.0a documentation seems to maintain global anchors if the anchor starts with _ */
                    if (*xp == '_') {
                        name = (char*)xp;
                        xmlFree(xp);
                    }
                }
            }

            if (!name.empty()) {
                fprintf(cat_fp,"%s\n",path);
                fprintf(cat_fp,"-name:%s\n",name.c_str());
                fprintf(cat_fp,"\n");
            }
        }

        lookForAnchors(node->children,path);
        node = node->next;
    }
}

void GetHelpId(const char *path) {
    xmlNodePtr html=NULL,body=NULL,scan=NULL;
    xmlDocPtr doc;

    /* NTS: MSHC are generally XHTML */

    doc = htmlParseFile(path,NULL);
    if (doc == NULL) return;

    /* look for the HTML -> HEAD -> META entries */
    html = xmlDocGetRootElement(doc);
    while (html != NULL) {
        if (!xmlStrcmp(html->name,(const xmlChar*)"html"))
            break;

        html = html->next;
    }

    if (html != NULL) {
        body = html->children;
        while (body != NULL) {
            if (!xmlStrcmp(body->name,(const xmlChar*)"body"))
                break;

            body = body->next;
        }
    }

    lookForAnchors(body,path);

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

