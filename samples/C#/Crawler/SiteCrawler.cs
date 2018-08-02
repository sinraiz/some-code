using Crawler.Code;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Crawler
{
    public class SiteCrawler
    {
        public SiteCrawler()
        {

        }

        public void start()
        {
            if(!_isDone)
            {
                throw new Exception("The parser is already busy");
            }

            // Reset the flags
            _isDone = false;
            _stopRequested = false;
            stopEvent = new CancellationTokenSource();

            // Initiate the crawling
            parsePages();
        }

        public void stop()
        {
            // Only indicate the intention of 
            // a user to stop the whole thing
            _stopRequested = true;

            // Also make the workers stop
            stopEvent.Cancel();
        }

        private void doReport(MessageKind msgKind, string msgText)
        {
            // Depends on the message kind and if we have the handlers
            if (onReport != null && msgKind != MessageKind.Trace)
            {
                // Inform the caller only of a meaningful stuff
                onReport(msgKind, msgText);
            }
            else
            {
                // Trace and unhandled messages go to the debug console
                string debugMessage = String.Format("{0}: {1} - {2}",
                    DateTime.Now.ToShortTimeString(),
                    msgKind.ToString(),
                    msgText.ToString()
                    );
                System.Diagnostics.Debug.WriteLine(debugMessage);
            }
        }

        private void parsePages()
        {
            // Inform the world of us
            doReport(MessageKind.Info, "Parsing the page: " + Constants.SITE_URL);

            // Create the frontpage parser
            FrontPageParser frontParser = new FrontPageParser(Constants.SITE_URL);

            // Let the parser inform us
            frontParser.onReport += (msgKind, msgText) =>
            {
                doReport(msgKind, msgText);
            };

            // React on downloading completed
            frontParser.onDownloaded += (sender, data, isOk, error) =>
            {
                if(!isOk)
                {
                    doReport(MessageKind.Error, "Downloading failed: " + error);
                    return;
                }

                doReport(MessageKind.Info, "Downloading succeeded");
            };

            // React on index page parsing completed
            frontParser.onParsed += async (sender, headers, isOk, error) =>
            {
                if (!isOk)
                {
                    doReport(MessageKind.Error, "Index parsing failed: " + error);
                    return;
                }

                // Announce the results of index parsing
                doReport(MessageKind.Info, "Index page has been parsed successfully");
                doReport(MessageKind.Info, String.Format("{0} articles found:", headers.Count()));

                // Print headers
                printArticlesList(headers);

                // Proceed to pages parsing
                var data = parseArticles(headers);

                // Export and then red the data
                bool mongoRes = await mongoData(data, headers);
                if (!mongoRes)
                {
                    _isDone = true;
                    return;
                }
                _isDone = true;
            };

            // Initiate the index parsing
            frontParser.beginParse();
        }

        private void printArticlesList(IEnumerable<ArticleHeader> headers)
        {
            int articleIndex = 1;
            foreach(var header in headers)
            {
                // Print the title
                string title = String.Format("[9]Item {0,2}. {1}", articleIndex++, header.title);
                doReport(MessageKind.Result, title);

                // Print the rest
                doReport(MessageKind.Info, "[9]URL:     " + header.url);
                doReport(MessageKind.Info, "[9]Author:  " + header.authors);
                doReport(MessageKind.Info, "[9]Excerpt: " + header.summary);
                doReport(MessageKind.Info, "");
            }
        }

        private Dictionary<string, WordStatsInfo> parseArticles(IEnumerable<ArticleHeader> headers)
        {
            // Check for stop
            if(_stopRequested)
            {
                _isDone = true;
                return null;
            }
            doReport(MessageKind.Info, "Downloading articles...");

            // Push the articles into the queue
            Queue<ArticleHeader> tasks = new Queue<ArticleHeader>(headers);


            // Create the working threads and pass them all
            // the same collection of article headers to work on
            Task[] workers = new Task[Constants.MAX_THREADS];

            // The logger
            Action<MessageKind, string> reportAction = (k, m) => doReport(k, m);

            // The resulting global statistics
            var globalWordStats = new Dictionary<string, WordStatsInfo>();

            // Initiate the required number of workers and
            // make them toll
            for (int s = 0; s < Constants.MAX_THREADS; s++)
            {
                // Create the context for this worker
                ArticleParserContext context = new ArticleParserContext()
                {
                    id = s + 1,                                   // The worker's own index
                    shouldStop = stopEvent.Token,                 // So that a worker knows when to stop
                    articles = tasks,                             // Bunch of tasks they work on
                    reportAction = reportAction,                  // Let him report some messages
                    globalStats = globalWordStats                 // Where the workers puts statistics
                };

                // Start the worker and pass it its context
                workers[s] = Task.Factory.StartNew((state) =>
                {
                    // Start the work
                    ArticleParser.processArticles(state);
                },
                state: context);
            }

            // Wait for the threads to complete
            Task.WaitAll(workers);

            doReport(MessageKind.Result, "All workers finished");
            doReport(MessageKind.Info, "Processing the stats...");

            // Sort by frequencies
            var wordFrequencies = globalWordStats.OrderBy(c => c.Value.count)
                .ToList();
            //var wordFrequencies = globalWordStats.OrderByDescending(c => c.Value.count)
            //    .Take(Constants.TOP_WORDS_LIMIT)  // Sorry was too many of words otherwise
            //    .ToList();

            // Print the stats
            doReport(MessageKind.Result, "Count  | Word");
            doReport(MessageKind.Result, "-------|-----------------------------------------");
            foreach (var word in wordFrequencies)
            {
                doReport(MessageKind.Result, String.Format("{0} | {1}",
                    word.Value.count.ToString().PadLeft(6),
                    System.Threading.Thread.CurrentThread.CurrentCulture.TextInfo.ToTitleCase(word.Key)
                    ));
            }



            return globalWordStats;
        }

        private async Task<bool> mongoData(Dictionary<string, WordStatsInfo> data, IEnumerable<ArticleHeader> headers)
        {
            doReport(MessageKind.Info, "Saving data into Mongo DB...");

            // Init the Mongo interface
            var export = new MongoDataset()
            {
                doReport = this.doReport
            };

            // Save the pages to DB
            bool mongoRes = await export.saveWordStats(data);
            if(mongoRes)
            {
                doReport(MessageKind.Result, String.Format("{0} words saved into Mongo DB", data.Count));
            }
            else
            {
                doReport(MessageKind.Error, "Failed to save data into Mongo DB");
                return mongoRes;
            }

            doReport(MessageKind.Info, "Reading data from Mongo DB...");
            mongoRes = await export.printStats(headers.Skip(1).Take(1).First().url.ToString()); // Pass the second article's URL
            if (!mongoRes)
            {
                doReport(MessageKind.Error, "Failed to save data into Mongo DB");
            }

            return mongoRes;
        }

        #region Members & Properties

        public bool _isDone = true;
        public bool isDone
        {
            get
            {
                return _isDone;
            }
        }

        public bool _stopRequested = false;
        public CancellationTokenSource stopEvent = new CancellationTokenSource();

        #endregion

        #region Events
        public event Action<MessageKind, string> onReport;
        #endregion
    }
}
