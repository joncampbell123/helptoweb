
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

map<string,string>      catalogToFile;

FILE *cat_fp = NULL;
FILE *ls_fp = NULL;

unsigned char xdigit2(const char c) {
    if (c >= '0' && c <= '9')
        return (unsigned char)(c - '0');
    if (c >= 'a' && c <= 'f')
        return (unsigned char)(c + 10 - 'a');
    if (c >= 'A' && c <= 'F')
        return (unsigned char)(c + 10 - 'A');

    return 0;
}

string lowercase(string f) {
    string r;
    const char *s;

    s = f.c_str();
    while (*s != 0) {
        r += tolower(*s++);
    }

    return r;
}

string htmlunescape(const char *s) {
    string r;

    while (*s != 0) {
        if (*s == '%') {
            unsigned char c = 0;

            s++;
            if (*s && isxdigit(*s))
                c += xdigit2(*s++) << 4;
            if (*s && isxdigit(*s))
                c += xdigit2(*s++);

            if (c != 0)
                r += c;
        }
        else {
            r += *s++;
        }
    }

    return r;
}

string final2html(string &f,const char *dst) {
    string r = f,prefix;

    {
        while (!strncmp(dst,"./",2))
            dst += 2;

        while (*dst != 0) {
            if (*dst == '/') {
                while (*dst == '/') dst++;

                if (*dst != 0)
                    prefix += "../";
            }
            else {
                dst++;
            }
        }
    }

    if (!strncmp(f.c_str(),"./FINAL/",8)) {
        r = prefix;
        r += "./HTML/";
        r += f.c_str()+8;
    }

    return r;
}

string replaceLink(string &url,const char *dst) {
    if (!url.empty() && !strncmp(url.c_str(),"ms-xhelp:///?Id=",16)) {
        string unescaped = lowercase(htmlunescape(url.c_str()+16));
        map<string,string>::iterator i = catalogToFile.find(unescaped);
        if (i != catalogToFile.end()) {
            /* replace the link */
            return final2html(i->second,dst);
        }
        else {
            fprintf(stderr,"  Unknown ms-xhelp '%s'\n",unescaped.c_str());
        }
    }

    return "";
}

void xlateLinks(xmlNodePtr node,const char *dst) {

    {
        bool again = false;
        xmlNodePtr x;

        do {
            again = false;
            for (x=node;x;x=x->next) {
                if (!strcasecmp((char*)x->name,"CodeSnippet")) {
                    xmlNodePtr np;
                    xmlNodePtr content;

                    np = xmlNewNode(NULL,(const xmlChar*)"code");
                    xmlSetProp(np,(const xmlChar*)"style",(const xmlChar*)"white-space: pre; display: block;");

                    xmlAddPrevSibling(x,np);
                    xmlUnlinkNode(x);

                    content = x->children;
                    xmlUnlinkNode(content);
                    xmlAddChild(np,content);

                    xmlFreeNode(x);
                    again = true;
                    break;
                }
            }
        } while (again);
    }

    while (node != NULL) {
        if (!xmlStrcmp(node->name,(const xmlChar*)"a")) {
            {
                xmlChar *xp;

                xp = xmlGetNoNsProp(node,(const xmlChar*)"href");
                if (xp != NULL) {
                    string url = (char*)xp;
                    string newurl;
                    xmlFree(xp);

                    newurl = replaceLink(url,dst);
                    if (!newurl.empty())
                        xmlSetProp(node,(const xmlChar*)"href",(const xmlChar*)newurl.c_str());
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
            xlateLinks(node->children,dst);

        node = node->next;
    }
}

void copytranslate(const char *src,const char *dst) {
    xmlNodePtr html;
    xmlDocPtr doc;

    doc = xmlReadFile(src,NULL,XML_PARSE_RECOVER|XML_PARSE_NONET);
    if (doc == NULL) return;

    html = xmlDocGetRootElement(doc);
    if (html != NULL) xlateLinks(html,dst);

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
    string filename,fileid;
    struct stat st;
    string newpath;
    char line[1024];

    ls_fp = popen("find","r");
    if (ls_fp == NULL) return 1;

    cat_fp = fopen("catalog.txt","r");
    if (cat_fp == NULL) return 1;
    while (!feof(cat_fp) && !ferror(cat_fp)) {
        if (fgets(line,sizeof(line)-1,cat_fp) == NULL) break;
        chomp(line);

        if (line[0] == 0) {
            if (!filename.empty() && !fileid.empty())
                catalogToFile[fileid] = filename;

            filename.clear();
            fileid.clear();
        }
        else if (filename.empty()) {
            filename = line;
        }
        else if (!strncmp(line,"-id:",4)) {
            {
                char *s = line+4;
                while (*s != 0) {
                    *s = tolower(*s);
                    s++;
                }
            }
            fileid = line+4;
        }
    }
    fclose(cat_fp);

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
//                    printf("Translate: %s\n",line);
//                    printf("       to: %s\n",newpath.c_str());
                    copytranslate(line,newpath.c_str());
                }
                else {
                    printf("Copying: %s\n",line);
//                    printf("     to: %s\n",newpath.c_str());
                    copyfile(line,newpath.c_str());
                }
            }
            else if (S_ISDIR(st.st_mode)) {
                mkdir(newpath.c_str(),0755);
            }
        }
    }

    fclose(ls_fp);
    return 0;
}

