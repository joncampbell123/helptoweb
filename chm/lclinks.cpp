
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

string replaceLink(string &url,const char *rpath) {
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

    /* convert absolute paths to relative.
     * some CHMs do use absolute paths (Microsoft DXMedia, MSDN 6.0a library) */
    if (r.length() > 0 && r.at(0) == '/') {
        const char *r_str = r.c_str();
        while (*r_str == '/') r_str++;

        const char *r_scan = rpath;
        string rfinal;

        while (*r_scan != 0) {
            if (!strncmp(r_scan,"./",2)) {
                r_scan += 2;
                continue;
            }
            if (*r_scan != '/') {
                r_scan++;
                continue;
            }

            rfinal += "../";

            while (*r_scan == '/')
                r_scan++;
        }

        rfinal += r_str;
//        fprintf(stderr,"Absolute to relative: '%s' -> '%s'\n",r.c_str(),rfinal.c_str());
        r = rfinal;
    }

    return r;
}

void xlateLinks(xmlDocPtr doc,xmlNodePtr node,const char *rpath) {
    while (node != NULL) {
        if (!xmlStrcmp(node->name,(const xmlChar*)"a")) {
            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"href");
                if (xp != NULL) {
                    string url = (char*)xp;
                    string newurl;
                    xmlFree(xp);

                    newurl = replaceLink(url,rpath);
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

                    newurl = replaceLink(url,rpath);
                    if (!newurl.empty())
                        xmlSetProp(node,(const xmlChar*)"src",(const xmlChar*)newurl.c_str());
                }
            }
        }
        else if (!xmlStrcmp(node->name,(const xmlChar*)"script")) {
            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"src");
                if (xp != NULL) {
                    string url = (char*)xp;
                    string newurl;
                    xmlFree(xp);

                    newurl = replaceLink(url,rpath);
                    if (!newurl.empty())
                        xmlSetProp(node,(const xmlChar*)"src",(const xmlChar*)newurl.c_str());
                }
            }
        }
        else if (!xmlStrcmp(node->name,(const xmlChar*)"link")) {
            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"href");
                if (xp != NULL) {
                    string url = (char*)xp;
                    string newurl;
                    xmlFree(xp);

                    newurl = replaceLink(url,rpath);
                    if (!newurl.empty())
                        xmlSetProp(node,(const xmlChar*)"href",(const xmlChar*)newurl.c_str());
                }
            }
        }
        else if (!xmlStrcmp(node->name,(const xmlChar*)"iframe")) {
            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"src");
                if (xp != NULL) {
                    string url = (char*)xp;
                    string newurl;
                    xmlFree(xp);

                    newurl = replaceLink(url,rpath);
                    if (!newurl.empty())
                        xmlSetProp(node,(const xmlChar*)"src",(const xmlChar*)newurl.c_str());
                }
            }
        }
        else if (!xmlStrcmp(node->name,(const xmlChar*)"style")) {
            xmlChar *val;

            val = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            if (val != NULL) {
                char *s = (char*)val;
                string nval = s;

                while (*s == ' ' || *s == '\t') s++;

                /* MSDN documentation: @import url(...) */
                if (!strncasecmp((char*)s,"@import url(",12)) {
                    nval = lowercase(s);
                }

                xmlFree(val);
                xmlNodeSetContent(node,(const xmlChar*)nval.c_str());
            }
        }
        else if (!xmlStrcmp(node->name,(const xmlChar*)"meta")) {
            /* need to remove content encoding since we change it */
            xmlChar *charset;
            xmlChar *httpequiv;
            xmlChar *content;

            charset = xmlGetProp(node,(const xmlChar*)"charset");
            httpequiv = xmlGetProp(node,(const xmlChar*)"http-equiv");
            content = xmlGetProp(node,(const xmlChar*)("content"));

            if (charset != NULL) {
                // if this isn't our UTF-8 charset tag then remove it
                if (strcasecmp((char*)charset,"UTF-8") != 0) {
                    xmlNodePtr n = node->next;
                    xmlUnlinkNode(node);
                    xmlFreeNode(node);
                    node = n;
                    continue;
                }
            }
            else if (httpequiv != NULL && content != NULL) {
                // if this isn't our UTF-8 charset tag then remove it
                if (strcasecmp((char*)httpequiv,"Content-Type") == 0 && strcasecmp((char*)content,"text/html; charset=utf-8") != 0) {
                    xmlNodePtr n = node->next;
                    xmlUnlinkNode(node);
                    xmlFreeNode(node);
                    node = n;
                    continue;
                }
            }
        }
        else if (!xmlStrcmp(node->name,(const xmlChar*)"head")) {
            /* need to add a meta tag that clarifies the document is UTF-8 */
            xmlNodePtr charset;

            charset = xmlNewNode(NULL,(const xmlChar*)"meta");
            xmlNewProp(charset,(const xmlChar*)"charset",(const xmlChar*)"UTF-8");
            xmlAddChild(node,charset);

            charset = xmlNewNode(NULL,(const xmlChar*)"meta");
            xmlNewProp(charset,(const xmlChar*)"http-equiv",(const xmlChar*)"Content-Type");
            xmlNewProp(charset,(const xmlChar*)"content",(const xmlChar*)"text/html; charset=utf-8");
            xmlAddChild(node,charset);
        }

        if (node->children)
            xlateLinks(doc,node->children,rpath);

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
    if (html != NULL) xlateLinks(doc,html,src);

    xmlSaveFileEnc(tmp.c_str(),doc,"UTF-8");
    xmlFreeDoc(doc);

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

