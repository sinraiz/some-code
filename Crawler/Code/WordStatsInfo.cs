using MongoDB.Bson;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Crawler.Code
{
    /**
        This object keeps the information of the total number
        of occurences of the word and list of articles which 
        contain it
    */
    public class WordStatsInfo
    {
        // The overall number of entries
        public int count { get; set; }

        // The list of article URLs where it was found
        public HashSet<string> articles = new HashSet<string>();
    }


    /**
        This object keeps the information of the total number
        of occurences of the word and list of articles which 
        contain it. This object is used for inserts into the 
        DB instance
    */
    public class DBWordInfo : BsonDocument
    {
        [MongoDB.Bson.Serialization.Attributes.BsonId]
        public string word { get; set; }

        // The overall number of entries
        public int count { get; set; }

        // The list of article URLs where it was found
        public List<string> articles { get; set; }
    }
}
