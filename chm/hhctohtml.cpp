
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
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

xmlDocPtr indexHTMLDoc = NULL;
xmlNodePtr indexHTML = NULL;
xmlNodePtr indexHTMLHead = NULL;
xmlNodePtr indexHTMLBody = NULL;

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

void changeSitemapsToLinks(xmlNodePtr parent_node) {
    xmlNodePtr node;

    for (node=parent_node;node;) {
        if (!strcasecmp((char*)node->name,"object")) {
            string type;

            {
                xmlChar *xp;
                xp = xmlGetNoNsProp(node,(const xmlChar*)"type");
                if (xp != NULL) {
                    type = (char*)xp;
                    xmlFree(xp);
                }
            }

            if (type == "text/sitemap") {
                string name,local;
                xmlNodePtr par;

                for (par=node->children;par;par=par->next) {
                    if (!strcasecmp((char*)par->name,"param")) {
                        string p_name,p_value;

                        {
                            xmlChar *xp;
                            xp = xmlGetNoNsProp(par,(const xmlChar*)"name");
                            if (xp != NULL) {
                                p_name = (char*)xp;
                                xmlFree(xp);
                            }
                        }

                        {
                            xmlChar *xp;
                            xp = xmlGetNoNsProp(par,(const xmlChar*)"value");
                            if (xp != NULL) {
                                p_value = (char*)xp;
                                xmlFree(xp);
                            }
                        }

                        if (p_name == "Name")
                            name = p_value;
                        else if (p_name == "Local")
                            local = p_value;
                    }
                }

                if (!name.empty()) {
                    xmlNodePtr alink;

                    if (!local.empty()) {
                        alink = xmlNewNode(NULL,(const xmlChar*)"a");
                        xmlNodeSetContent(alink,(const xmlChar*)name.c_str());
                        xmlNewProp(alink,(const xmlChar*)"href",(const xmlChar*)replaceLink(local).c_str());
                    }
                    else {
                        alink = xmlNewNode(NULL,(const xmlChar*)"span");
                        xmlNodeSetContent(alink,(const xmlChar*)name.c_str());
                    }

                    xmlAddPrevSibling(node,alink);
                }
            }
            else {
                fprintf(stderr,"Ignoring OBJECT type '%s'\n",type.c_str());
            }

            xmlNodePtr nn = node->next;
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            node = nn;
        }
        else {
            changeSitemapsToLinks(node->children);
            node = node->next;
        }
    }
}

void changeSitemapsToLinks(void) {
    changeSitemapsToLinks(indexHTMLBody);
}

bool parseHHC(const char *path) {
    xmlNodePtr html,node;
    xmlDocPtr doc;

    indexHTMLDoc = xmlNewDoc((const xmlChar*)"1.0");
    if (indexHTMLDoc == NULL) return false;

    indexHTML = xmlNewNode(NULL,(const xmlChar*)"html");
    if (indexHTML == NULL) return false;
    xmlDocSetRootElement(indexHTMLDoc,indexHTML);

    indexHTMLHead = xmlNewNode(NULL,(const xmlChar*)"head");
    if (indexHTMLHead == NULL) return false;
    xmlAddChild(indexHTML,indexHTMLHead);

    {
        xmlNodePtr a = xmlNewNode(NULL,(const xmlChar*)"meta");
        xmlNewProp(a,(const xmlChar*)"http-equiv",(const xmlChar*)"Content-Type");
        xmlNewProp(a,(const xmlChar*)"content",(const xmlChar*)"text/html; charset=UTF-8");
        xmlAddChild(indexHTMLHead,a);
    }

    {
        xmlNodePtr a = xmlNewNode(NULL,(const xmlChar*)"meta");
        xmlNewProp(a,(const xmlChar*)"charset",(const xmlChar*)"UTF-8");
        xmlAddChild(indexHTMLHead,a);
    }

    indexHTMLBody = xmlNewNode(NULL,(const xmlChar*)"body");
    if (indexHTMLBody == NULL) return false;
    xmlAddChild(indexHTML,indexHTMLBody);

    /* <HTML>
     *   <UL>
     *    <LI><OBJECT type="text/sitemap">
     *      <PARAM name="Name" value="COM">
     *      </OBJECT>
     *      <UL>
     *        <LI><OBJECT type="text/sitemap">
     *          <PARAM name="Name" value="Legal Information">
     *          <PARAM name="Local" value="devdoc/live/com/legalole_6q3y.htm">
     *        </OBJECT>
     *      </UL>
     *   </UL>
     * </HTML> */

    doc = htmlParseFile(path,NULL);
    if (doc == NULL) return false;

    html = xmlDocGetRootElement(doc);
    if (html != NULL) {
        xmlNodePtr body = NULL;

        /* libxml2 automatically makes a body even if HHC files don't have one */
        for (node=html->children;node;node=node->next) {
            if (!strcasecmp((char*)node->name,"body")) {
                body = node;
                break;
            }
        }

        if (body == NULL)
            body = html;

        /* copy only the UL or OL nodes */
        for (node=body->children;node;node=node->next) {
            if (!strcasecmp((char*)node->name,"ul") || !strcasecmp((char*)node->name,"ol")) {
                xmlNodePtr copy = xmlCopyNode(node,1);
                if (copy != NULL)
                    xmlAddChild(indexHTMLBody,copy);
                else
                    fprintf(stderr,"Failed to copy link node\n");
            }
            else {
                fprintf(stderr,"Skipping %s\n",(char*)node->name);
            }
        }
    }

    xmlFreeDoc(doc);
    return true;
}

int main() {
    DIR *dir;
    string hhc_file;
    string hhk_file; // TODO: what do to?
    struct dirent *d;
    struct stat st;
    char line[1024];

    dir = opendir(".");
    if (dir == NULL) return 1;
    while ((d=readdir(dir)) != NULL) {
        if (d->d_name[0] == '.') continue;

        char *ext = strrchr(d->d_name,'.');
        if (ext == NULL) continue;

        if (!strcasecmp(ext,".hhc")) {
            if (hhc_file.empty())
                hhc_file = d->d_name;
        }
        else if (!strcasecmp(ext,".hhk")) {
            if (hhk_file.empty())
                hhk_file = d->d_name;
        }
    }
    closedir(dir);

    if (hhc_file.empty()) {
        fprintf(stderr,"no hhc file found\n");
        return 1;
    }

    printf("HHC file: %s\n",hhc_file.c_str());
    printf("HHK file: %s\n",hhk_file.c_str());

    if (!parseHHC(hhc_file.c_str()))
        return 1;

    /* now, change the OBJECTs to links */
    changeSitemapsToLinks();

    xmlSaveFileEnc("hhc_toc.htm",indexHTMLDoc,"UTF-8");
    xmlFreeDoc(indexHTMLDoc);
    indexHTMLDoc = NULL;
    return 0;
}

