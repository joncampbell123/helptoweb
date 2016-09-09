/* run this at the root of the extracted CHM contents directory to fix the collapsable sections in the Microsoft June 2010 DirectX SDK.
 * this fix is needed to browse the documentation on non-IE browsers. */

#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <libxml/parser.h>
#include <libxml/HTMLtree.h>
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

// look for DIVs marked as collapsable sections.
// if the style attribute contains "display: none" then remove the display: none style.
// this makes the sections appear by default.
void removeSectionDisplayNone(xmlNodePtr node) {
    while (node) {
        if (node->name == NULL) {
        }
        else if (!strcasecmp((char*)node->name,"div")) {
            string div_class,div_name,div_style;

            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"class");
                if (xp != NULL) {
                    div_class = (char*)xp;
                    div_class = lowercase(div_class);
                    xmlFree(xp);
                }
            }

            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"name");
                if (xp != NULL) {
                    div_name = (char*)xp;
                    div_name = lowercase(div_name);
                    xmlFree(xp);
                }
            }

            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"style");
                if (xp != NULL) {
                    div_style = (char*)xp;
                    div_style = lowercase(div_style);
                    xmlFree(xp);
                }
            }

            if (div_class == "section" && div_name == "collapseablesection") {
                const char *a_display = strstr(div_style.c_str(),"display:");
                if (a_display != NULL) {
                    const char *start = a_display;
                    const char *end = NULL;
                    const char *s = start + 8;
                    while (*s == ' ') s++;
                    if (!strncmp(s,"none;",5)) {
                        end = s + 5;
                        while (*end == ' ') end++;

                        fprintf(stderr,"DIV: removing display: none for section\n");

                        string ndiv = string(div_style.c_str(),(size_t)(start - div_style.c_str()));
                        ndiv += string(end);

                        xmlSetProp(node,(const xmlChar*)"style",(const xmlChar*)ndiv.c_str());
                    }
                }
            }
        }

        removeSectionDisplayNone(node->children);
        node = node->next;
    }
}

// look for label elements marked "collapse all" and remove them
void removeCollapseAll(xmlNodePtr node) {
    while (node) {
        if (node->name == NULL) {
        }
        else if (!strcasecmp((char*)node->name,"label")) {
            string label_id;

            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"id");
                if (xp != NULL) {
                    label_id = (char*)xp;
                    label_id = lowercase(label_id);
                    xmlFree(xp);
                }
            }

            if (label_id == "collapsealllabel" || label_id == "expandalllabel") {
                xmlNodePtr n = node->next;
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                node = n;
                continue;
            }
        }

        removeCollapseAll(node->children);
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
    if (html != NULL) {
        removeSectionDisplayNone(html);
        removeCollapseAll(html);
    }

    {
        FILE *fp = fopen(tmp.c_str(),"w");
        if (fp == NULL) abort();
        htmlDocDump(fp,doc);
        fclose(fp);
    }
    xmlFreeDoc(doc);

    return;

    unlink(src);
    rename(tmp.c_str(),src);
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

