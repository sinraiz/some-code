using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Crawler.Code
{
    /** 
    Defines the article header's contents as parsed on
    the front page
    */
    public class ArticleHeader
    {
        public string title { get; set; }
        public Uri url { get; set; }
        public string authors { get; set; }
        public string summary { get; set; }
    }

}
