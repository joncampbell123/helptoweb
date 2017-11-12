/* run this at the root of the extracted CHM contents directory */

#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <libxml/parser.h>
#include <libxml/HTMLtree.h>
#include <libxml/HTMLparser.h>

#include <map>
#include <string>
#include <vector>

using namespace std;

static void chomp(char *s) {
    char *e = s + strlen(s) - 1;

    while (e >= s && (*e == '\r' || *e == '\n')) *e-- = 0;
}

string mainpageurl;

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
    bool has_alink = false;
    xmlNodePtr node;

    for (node=parent_node;node;) {
        if (!strcasecmp((char*)node->name,"object")) {
            string type;

            {
                xmlChar *xp;
                xp = xmlGetNoNsProp(node,(const xmlChar*)"type");
                if (xp != NULL) {
                    type = (char*)xp;
                    type = lowercase(type);
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
                        {
                            const char *s = local.c_str();

                            /* some HHCs use ../../ in the URL despite the fact that
                             * from within a CHM that just refers to the root "directory" anyway
                             * (Microsoft Speech API SDK) */
                            while (!strncmp(s,"../",3)) s += 3;

                            if (s != local.c_str()) {
                                string ns = s;
                                local = ns;
                            }
                        }

                        alink = xmlNewNode(NULL,(const xmlChar*)"a");
                        xmlNodeSetContent(alink,(const xmlChar*)name.c_str());
                        xmlNewProp(alink,(const xmlChar*)"href",(const xmlChar*)replaceLink(local).c_str());
                        xmlNewProp(alink,(const xmlChar*)"target",(const xmlChar*)"chm_contentframe");

                        if (mainpageurl.empty())
                            mainpageurl = replaceLink(local);
                    }
                    else {
                        alink = xmlNewNode(NULL,(const xmlChar*)"span");
                        xmlNodeSetContent(alink,(const xmlChar*)name.c_str());
                    }

                    xmlAddPrevSibling(node,alink);
                    has_alink = true;
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
        else if (!strcasecmp((char*)node->name,"a")) {
            if (has_alink) { // here's one of Microsoft's curveballs: some HHC files have both an object and an anchor. Blah.
                xmlNodePtr nn = node->next;
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                node = nn;
            }
            else {
                node = node->next;
            }
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

void changeKeywordSitemapsToLinks(xmlNodePtr parent_node) {
    bool has_alink = false;
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
                vector<string> name,local;
                string keyword,see_also;
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
                            name.push_back(p_value);
                        else if (p_name == "Local")
                            local.push_back(p_value);
                        else if (p_name == "Keyword")
                            keyword = p_value;
                        else if (p_name == "See Also")
                            see_also = p_value;
                    }
                }

                /* Some HHKs define Name twice, the first time the same meaning as Keyword, apparently (Microsoft SAPI 5.1 SDK) */
                if (keyword.empty() && name.size() > local.size() && name.size() != 0) {
                    keyword = name.front();
                    name.erase(name.begin());
                }

                if (!keyword.empty()) {
                    xmlNodePtr alink,ul=NULL;
                    size_t i;

                    /* NTS: Some HHKs have a Keyword and Local, no Name */
                    if (name.size() == 0 && local.size() == 1) {
                        string k_local = local[0];

                        {
                            const char *s = k_local.c_str();

                            /* some HHCs use ../../ in the URL despite the fact that
                             * from within a CHM that just refers to the root "directory" anyway
                             * (Microsoft Speech API SDK) */
                            while (!strncmp(s,"../",3)) s += 3;

                            if (s != k_local.c_str()) {
                                string ns = s;
                                k_local = ns;
                            }
                        }
 
                        alink = xmlNewNode(NULL,(const xmlChar*)"a");
                        xmlNodeSetContent(alink,(const xmlChar*)keyword.c_str());
                        xmlNewProp(alink,(const xmlChar*)"href",(const xmlChar*)replaceLink(k_local).c_str());
                        xmlNewProp(alink,(const xmlChar*)"target",(const xmlChar*)"chm_contentframe");
                        xmlAddPrevSibling(node,alink);
                        has_alink = true;
                   }
                    else {
                        alink = xmlNewNode(NULL,(const xmlChar*)"span");
                        xmlNodeSetContent(alink,(const xmlChar*)keyword.c_str());
                        xmlAddPrevSibling(node,alink);
                        has_alink = true;
                    }

                    for (i=0;i < name.size() && i < local.size();i++) {
                        string k_name = name[i];
                        string k_local = local[i];
                        xmlNodePtr a,li;

                        if (i == 0) {
                            ul = xmlNewNode(NULL,(const xmlChar*)"ul");
                            xmlAddChild(alink,ul);
                        }

                        li = xmlNewNode(NULL,(const xmlChar*)"li");
                        xmlAddChild(ul,li);

                        {
                            const char *s = k_local.c_str();

                            /* some HHCs use ../../ in the URL despite the fact that
                             * from within a CHM that just refers to the root "directory" anyway
                             * (Microsoft Speech API SDK) */
                            while (!strncmp(s,"../",3)) s += 3;

                            if (s != k_local.c_str()) {
                                string ns = s;
                                k_local = ns;
                            }
                        }
 
                        a = xmlNewNode(NULL,(const xmlChar*)"a");
                        xmlNodeSetContent(a,(const xmlChar*)k_name.c_str());
                        xmlNewProp(a,(const xmlChar*)"href",(const xmlChar*)replaceLink(k_local).c_str());
                        xmlNewProp(a,(const xmlChar*)"target",(const xmlChar*)"chm_contentframe");
                        xmlAddChild(li,a);
                    }
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
        else if (!strcasecmp((char*)node->name,"a")) {
            if (has_alink) { // here's one of Microsoft's curveballs: some HHC files have both an object and an anchor. Blah.
                xmlNodePtr nn = node->next;
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                node = nn;
            }
            else {
                node = node->next;
            }
        }
        else {
            changeKeywordSitemapsToLinks(node->children);
            node = node->next;
        }
    }
}

void changeKeywordSitemapsToLinks(void) {
    changeKeywordSitemapsToLinks(indexHTMLBody);
}

bool makeFramePage(void) {
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
    xmlNewProp(indexHTMLBody,(const xmlChar*)"style",(const xmlChar*)"padding: 0px; margin: 0px;");
    xmlAddChild(indexHTML,indexHTMLBody);

    {
        xmlNodePtr div = xmlNewNode(NULL,(const xmlChar*)"div");
        xmlNewProp(div,(const xmlChar*)"style",(const xmlChar*)"float: left; width: 35%;");
        {
            xmlNodePtr a = xmlNewNode(NULL,(const xmlChar*)"iframe");
            xmlNewProp(a,(const xmlChar*)"frameborder",(const xmlChar*)"0");
            xmlNewProp(a,(const xmlChar*)"width",(const xmlChar*)"100%");
            xmlNewProp(a,(const xmlChar*)"height",(const xmlChar*)"100%");
            xmlNewProp(a,(const xmlChar*)"src",(const xmlChar*)"hhc_toc.htm");
            xmlNewProp(a,(const xmlChar*)"name",(const xmlChar*)"chm_tocframe");
            xmlNewProp(a,(const xmlChar*)"id",(const xmlChar*)"chm_tocframe");
            xmlNodeSetContent(a,(const xmlChar*)"Your browser does not support IFRAMEs");
            xmlAddChild(div,a);
        }
        xmlAddChild(indexHTMLBody,div);
    }

    if (!mainpageurl.empty()) {
        xmlNodePtr div = xmlNewNode(NULL,(const xmlChar*)"div");
        xmlNewProp(div,(const xmlChar*)"style",(const xmlChar*)"float: left; width: 65%;");
        {
            xmlNodePtr a = xmlNewNode(NULL,(const xmlChar*)"iframe");
            xmlNewProp(a,(const xmlChar*)"frameborder",(const xmlChar*)"0");
            xmlNewProp(a,(const xmlChar*)"width",(const xmlChar*)"100%");
            xmlNewProp(a,(const xmlChar*)"height",(const xmlChar*)"100%");
            xmlNewProp(a,(const xmlChar*)"src",(const xmlChar*)mainpageurl.c_str());
            xmlNewProp(a,(const xmlChar*)"name",(const xmlChar*)"chm_contentframe");
            xmlNewProp(a,(const xmlChar*)"id",(const xmlChar*)"chm_contentframe");
            xmlNodeSetContent(a,(const xmlChar*)"Your browser does not support IFRAMEs");
            xmlAddChild(div,a);
        }
        xmlAddChild(indexHTMLBody,div);
    }

    return true;
}

bool init_HTML(void) {
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

    return true;
}

bool parseHHK(const char *path) {
    xmlNodePtr html,node;
    xmlDocPtr doc;

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

bool parseHHC(const char *path) {
    xmlNodePtr html,node;
    xmlDocPtr doc;

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

    if (!init_HTML())
        return 1;

    if (!parseHHC(hhc_file.c_str()))
        return 1;

    /* now, change the OBJECTs to links */
    changeSitemapsToLinks();

    if (!hhk_file.empty()) {
        {
            xmlNodePtr a = xmlNewNode(NULL,(const xmlChar*)"hr");
            xmlAddChild(indexHTMLBody,a);
        }

        if (!parseHHK(hhk_file.c_str()))
            return 1;

        /* now, change the OBJECTs to links */
        changeKeywordSitemapsToLinks();
    }

    printf("Main page: %s\n",mainpageurl.c_str());

    {
        FILE *fp = fopen("hhc_toc.htm","w");
        if (fp == NULL) abort();
        htmlDocDump(fp,indexHTMLDoc);
        fclose(fp);
    }
    xmlFreeDoc(indexHTMLDoc);
    indexHTMLDoc = NULL;

    /* make another HTML that joins the TOC with the content */
    makeFramePage();
    {
        FILE *fp = fopen("hhc_index.htm","w");
        if (fp == NULL) abort();
        htmlDocDump(fp,indexHTMLDoc);
        fclose(fp);
    }
    xmlFreeDoc(indexHTMLDoc);
    indexHTMLDoc = NULL;

    return 0;
}

