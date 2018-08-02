using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Crawler
{
    /**
        Keep the all the mutable configurations in one place
    */
    public class Constants
    {
        public static readonly string SITE_URL = "http://techcrunch.com/";
        public static readonly int MAX_ARTICLES = 20;
        public static readonly string SIGNATURE_POST = "post-title";
        public static readonly string SIGNATURE_AUTHORS = "byline";
        public static readonly string SIGNATURE_SUMMARY = "excerpt";
        public static readonly string SIGNATURE_CONTENT = "article-entry";
        public static readonly int MAX_THREADS = 3;
        public static readonly int TOP_WORDS_LIMIT = 50;
        public static readonly string MONGODB_CONNECT = "mongodb://localhost";
        
    }
}
