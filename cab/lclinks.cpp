
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

#include <map>
#include <string>

using namespace std;

static void chomp(char *s) {
    char *e = s + strlen(s) - 1;

    while (e >= s && (*e == '\r' || *e == '\n')) *e-- = 0;
}

FILE *ls_fp = NULL;

string lowercase(string f) {
    string r;
    const char *s;

    s = f.c_str();
    while (*s != 0) {
        r += tolower(*s++);
    }

    return r;
}

string replaceLink(string &url) {
    const char *s = url.c_str();
    const char *h = strchr(s,'#');
    string r;

    if (h != NULL) {
        r += lowercase(string(s,(size_t)(h-s)));
        r += h;
    }
    else {
        r += lowercase(s);
    }

    return r;
}

void xlateLinks(xmlNodePtr node) {
    while (node != NULL) {
        if (!xmlStrcmp(node->name,(const xmlChar*)"a")) {
            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"href");
                if (xp != NULL) {
                    string url = (char*)xp;
                    string newurl;
                    xmlFree(xp);

                    newurl = replaceLink(url);
                    if (!newurl.empty())
                        xmlSetProp(node,(const xmlChar*)"href",(const xmlChar*)newurl.c_str());
                }
            }
        }
        else if (!xmlStrcmp(node->name,(const xmlChar*)"img")) {
            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"src");
                if (xp != NULL) {
                    string url = (char*)xp;
                    string newurl;
                    xmlFree(xp);

                    newurl = replaceLink(url);
                    if (!newurl.empty())
                        xmlSetProp(node,(const xmlChar*)"src",(const xmlChar*)newurl.c_str());
                }
            }
        }
 
        if (node->children)
            xlateLinks(node->children);

        node = node->next;
    }
}

void copytranslate(const char *src) {
    xmlNodePtr html;
    xmlDocPtr doc;
    string tmp;

    tmp = src;
    tmp += ".tmp.xyz.tmp";

    doc = htmlParseFile(src,NULL);
    if (doc == NULL) return;

    html = xmlDocGetRootElement(doc);
    if (html != NULL) xlateLinks(html);

    xmlSaveFileEnc(tmp.c_str(),doc,"UTF-8");
    xmlFreeDoc(doc);
}

int main() {
    string filename,fileid;
    struct stat st;
    char line[1024];

    ls_fp = popen("find","r");
    if (ls_fp == NULL) return 1;

    while (!feof(ls_fp) && !ferror(ls_fp)) {
        if (fgets(line,sizeof(line)-1,ls_fp) == NULL) break;
        chomp(line);

        /* must be .htm or .html */
        char *ext = strrchr(line,'.');
        if (ext == NULL) continue;

        if (lstat(line,&st)) continue;

        if (S_ISREG(st.st_mode)) {
            if (!strcmp(ext,".htm") || !strcmp(ext,".html"))
                copytranslate(line);
        }
    }

    fclose(ls_fp);
    return 0;
}

