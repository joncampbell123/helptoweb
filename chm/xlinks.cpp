/* run this at the top of the same directory you built catalog.txt */

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
#include <vector>
#include <string>

using namespace std;

map<string,string>      anchor2path;    /* anchor name to CHM path (with an anchor of that name) */

static void chomp(char *s) {
    char *e = s + strlen(s) - 1;

    while (e >= s && (*e == '\r' || *e == '\n')) *e-- = 0;
}

FILE *ls_fp = NULL;

/* per file only */

/* objects[id] = pair(target,node) */
map<string,pair<string,xmlNodePtr> >    anchor_alink_objects;       /* <OBJECT> tags, Microsoft ActiveX links to cross-link CHMs */

/* anchors[] = pair(target,anchornode) */
vector<pair<string,xmlNodePtr> >        anchor_alink_anchors;       /* <A> tags that refer to the <OBJECT> tags (<A HREF="javascript:objectbyname.click()">) */

string lowercase(string f) {
    string r;
    const char *s;

    s = f.c_str();
    while (*s != 0) {
        r += tolower(*s++);
    }

    return r;
}

void ALinksTranslate(const char *path) {
    /* go through the nodes we collected and change the href to the actual file, then remove the OBJECT node.
     * if we can't identify which OBJECT goes to which anchor, then we don't change anything about either node. */
    vector<pair<string,xmlNodePtr> >::iterator i;
    string rfinal;

    {
        const char *r_scan = path;

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
    }

    for (i=anchor_alink_anchors.begin();i!=anchor_alink_anchors.end();i++) {
        string name = (*i).first;
        xmlNodePtr anchor_node = (*i).second;
        xmlNodePtr object_node = NULL;
        string anchor_name;

        {
            map<string,pair<string,xmlNodePtr> >::iterator j = anchor_alink_objects.find(name);
            if (j != anchor_alink_objects.end()) {
                object_node = j->second.second;
                anchor_name = j->second.first;
            }
        }

        if (object_node == NULL) {
            fprintf(stderr,"ALink for '%s', object not found in %s\n",name.c_str(),path);
            continue;
        }

        string chm_path;
        {
            map<string,string>::iterator ii = anchor2path.find(anchor_name);
            if (ii == anchor2path.end()) {
                fprintf(stderr,"No such ALink target in category for '%s' in %s\n",anchor_name.c_str(),path);
                continue;
            }

            chm_path = ii->second;
        }

        /* remove the object */
        xmlUnlinkNode(object_node);

        /* replace the HREF with the file.
         * make sure to tack on the anchor name. */
        string new_url = rfinal + chm_path + '#' + anchor_name;
        xmlSetProp(anchor_node,(const xmlChar*)"href",(const xmlChar*)new_url.c_str());

        fprintf(stderr,"'%s' = '%s' = '%s'\n",name.c_str(),anchor_name.c_str(),new_url.c_str());
    }
}

void buildListOfALinks(xmlDocPtr doc,xmlNodePtr node,const char *rpath) {
    while (node != NULL) {
        if (node->name == NULL) {
        }
        else if (!strcmp((char*)node->name,"a")) {
            /* we're looking for <A HREF="javascript:objectbyname.click()">....</A> */
            string href;

            {
                xmlChar *xp;
                xp = xmlGetNoNsProp(node,(const xmlChar*)"href");
                if (xp != NULL) {
                    href = (char*)xp;
                    xmlFree(xp);
                }
            }

            {
                const char *s = href.c_str();

                if (!strncasecmp(s,"javascript:",11)) {
                    s += 11;

                    /* do we see a name followed by .click() ? */
                    const char *e = strchr(s,'.');
                    if (e != NULL && strcasecmp(e,".click()") == 0) {
                        string objname = string(s,(size_t)(e-s));
                        if (!objname.empty())
                            anchor_alink_anchors.push_back(pair<string,xmlNodePtr>(objname,node));
                    }
                }
            }
        }
        else if (!strcmp((char*)node->name,"object")) {
            string object_id,object_type,object_classid;

            {
                xmlChar *xp;
                xp = xmlGetNoNsProp(node,(const xmlChar*)"id");
                if (xp != NULL) {
                    object_id = (char*)xp;
                    xmlFree(xp);
                }
            }

            {
                xmlChar *xp;
                xp = xmlGetNoNsProp(node,(const xmlChar*)"type");
                if (xp != NULL) {
                    object_type = (char*)xp;
                    xmlFree(xp);
                }
            }

            {
                xmlChar *xp;
                xp = xmlGetNoNsProp(node,(const xmlChar*)"classid");
                if (xp != NULL) {
                    object_classid = (char*)xp;
                    xmlFree(xp);
                }
            }

            if (object_type == "application/x-oleobject" || object_classid == "clsid:adb880a6-d8ff-11cf-9377-00aa003b7a11") {
                /* Microsoft alink (MSDN 6.0a library cross-CHM linking) */
                string param_Command;
                string param_Item1;
                string param_Item2;

                /* look at the <param> tags within */
                xmlNodePtr pnode = node->children;
                while (pnode != NULL) {
                    if (pnode->name == NULL) {
                    }
                    else if (!strcmp((char*)pnode->name,"param")) {
                        string p_name,p_value;

                        {
                            xmlChar *xp;
                            xp = xmlGetNoNsProp(pnode,(const xmlChar*)"name");
                            if (xp != NULL) {
                                p_name = (char*)xp;
                                xmlFree(xp);
                            }
                        }

                        {
                            xmlChar *xp;
                            xp = xmlGetNoNsProp(pnode,(const xmlChar*)"value");
                            if (xp != NULL) {
                                p_value = (char*)xp;
                                xmlFree(xp);
                            }
                        }

                        if (p_name == "Command")
                            param_Command = lowercase(p_value);
                        else if (p_name == "Item1")
                            param_Item1 = p_value;
                        else if (p_name == "Item2")
                            param_Item2 = p_value;
                    }

                    pnode = pnode->next;
                }

                if (param_Command == "alink" || param_Command == "alink,menu") {
                    if (param_Item1 == "") {
                        map<string,pair<string,xmlNodePtr> >::iterator i;

//                        fprintf(stderr,"ALink to '%s' as '%s' in %s\n",param_Item2.c_str(),object_id.c_str(),rpath);

                        i = anchor_alink_objects.find(object_id);
                        if (i == anchor_alink_objects.end()) {
                            anchor_alink_objects[object_id] = pair<string,xmlNodePtr>(param_Item2,node);
                        }
                        else {
                            fprintf(stderr,"WARNING: multiple objects for named anchor ALink '%s' defined\n",object_id.c_str());

                            xmlNodePtr n = node->next;
                            xmlUnlinkNode(node);
                            xmlFreeNode(node);
                            node = n;
                            continue;
                        }
                    }
                }
            }
        }

        if (node->children)
            buildListOfALinks(doc,node->children,rpath);

        node = node->next;
    }
}

void copytranslate(const char *src) {
    xmlNodePtr html;
    xmlDocPtr doc;
    string tmp;

    anchor_alink_objects.clear();
    anchor_alink_anchors.clear();

    tmp = src;
    tmp += ".tmp.xyz.tmp";

    doc = htmlParseFile(src,NULL);
    if (doc == NULL) return;

    html = xmlDocGetRootElement(doc);
    if (html != NULL) buildListOfALinks(doc,html,src);
    ALinksTranslate(src);

    anchor_alink_objects.clear();
    anchor_alink_anchors.clear();

    {
        FILE *fp = fopen(tmp.c_str(),"w");
        if (fp == NULL) abort();
        htmlDocDump(fp,doc);
        fclose(fp);
    }
    xmlFreeDoc(doc);

    unlink(src);
    rename(tmp.c_str(),src);
}

int main() {
    string filename,fileid;
    struct stat st;
    char line[1024];

    {
        string path,name;
        char *s;

        FILE *fp = fopen("catalog.txt","r");
        if (fp == NULL) return 1;
        while (!feof(fp) && !ferror(fp)) {
            if (fgets(line,sizeof(line)-1,fp) == NULL) break;
            chomp(line);
            s = line;

            if (*s == 0) {
                if (!path.empty() && !name.empty()) {
                    map<string,string>::iterator i = anchor2path.find(name);
                    if (i == anchor2path.end())
                        anchor2path[name] = path;
                    else
                        fprintf(stderr,"Ignoring duplicate named anchor %s\n",name.c_str());
                }

                path.clear();
                name.clear();
            }
            else if (path.empty()) {
                path = line;
            }
            else if (!strncmp(s,"-name:",6)) {
                s += 6;
                {
                    char *e = s;
                    while (*e != 0) {
                        *e = tolower(*e);
                        e++;
                    }
                }
                name = s;
            }
        }
        fclose(fp);
    }

    ls_fp = popen("find","r");
    if (ls_fp == NULL) return 1;

    while (!feof(ls_fp) && !ferror(ls_fp)) {
        if (fgets(line,sizeof(line)-1,ls_fp) == NULL) break;
        chomp(line);

        /* must be .htm or .html */
        char *ext = strrchr(line,'.');
        if (ext == NULL) continue;

//        if (strncmp(line,"./gdi.chm/",10)) continue;

        if (lstat(line,&st)) continue;

        if (S_ISREG(st.st_mode)) {
            if (!strcmp(ext,".htm") || !strcmp(ext,".html"))
                copytranslate(line);
        }
    }

    fclose(ls_fp);
    return 0;
}

