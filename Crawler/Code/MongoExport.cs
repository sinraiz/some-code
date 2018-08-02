using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MongoDB.Bson;
using MongoDB.Driver;

namespace Crawler.Code
{
    public class MongoDataset
    {
        public MongoDataset()
        {
            // Connect to local DB
            var client = new MongoClient(Constants.MONGODB_CONNECT);
            var database = client.GetDatabase("temp");
            tblWords = database.GetCollection<BsonDocument>("words");
        }

        public async Task<bool> saveWordStats(Dictionary<string, WordStatsInfo> data)
        {
            foreach (var entry in data)
            {
                // Preprare the doc to save
                var document = new BsonDocument
                {
                    { "_id", entry.Key}, // word itself as a key
                    { "count", entry.Value.count },
                    { "article_links", new BsonArray(entry.Value.articles) }
                };


                try
                {
                    await tblWords.ReplaceOneAsync(
                        filter: new BsonDocument("_id", entry.Key),
                        options: new UpdateOptions { IsUpsert = true },
                        replacement: document);
                }
                catch
                {
                    return false;
                }
            }

            return true;
        }

        public async Task<bool> printStats(string articleToFind)
        {
            // Find words with more than 200 entries
            var filter = Builders<BsonDocument>.Filter.Gte("count", 200);
            var frequentWords = await tblWords.Find(filter).ToListAsync();

            if(doReport!=null)
            {
                doReport(MessageKind.Result, String.Format("{0} words with more than 200 entries", frequentWords.Count));
            }

            // Find words that are unique and belong to article #2
            filter = Builders<BsonDocument>.Filter.Eq("count", 1) & Builders<BsonDocument>.Filter.AnyEq("article_links", articleToFind);
            var uniqueWords = await tblWords.Find(filter).ToListAsync();
            if (doReport != null)
            {
                doReport(MessageKind.Result, String.Format("{0} unique words within article #2", uniqueWords.Count));
            }

            return true;
        }

        public Action<MessageKind, string> doReport { get; set; }
        private IMongoCollection<BsonDocument> tblWords { get; set; }
    }
}
