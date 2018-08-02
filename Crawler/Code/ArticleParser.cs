using HtmlAgilityPack;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web;

namespace Crawler.Code
{
    /**
        This class contains implementation of a single worker peeking an article from the queue,
        then downloading and parsing it
    */
    public class ArticleParser
    {
        public static void processArticles(object context)
        {
            ArticleParserContext ctx = (context as ArticleParserContext);
            if(ctx == null)
            {
                return; // Shouldn't be here
            }

            ctx.reportAction(MessageKind.Info, String.Format("Worker {0}: Started", ctx.id));

            while (!ctx.shouldStop.IsCancellationRequested)
            {
                // Prepare to acquire a task from the shared queue
                ArticleHeader task = null;

                // Try to pick a task from the queue. For this
                // we need to seriliaze access to the queue among
                // all the workers, so we use a simple critical section
                lock (ctx.articles)
                {
                    // Check if the queue is not empty
                    if (ctx.articles.Count > 0)
                    {
                        // Take a task from it
                        task = ctx.articles.Dequeue();
                    }
                }

                // Now either we acquired a task or there are no more tasks
                if(task == null || ctx.shouldStop.IsCancellationRequested)
                {
                    break; // exit the working loop
                }

                // Report the allocation. Due to the multiline output
                // we will have to lock other workers attempt to print
                // at the same time
                lock(ctx.reportAction)
                {
                    ctx.reportAction(MessageKind.Info, String.Format("[10]Worker {0}: Parsing the article \"{1}\"", ctx.id, task.title));
                }

                // Process the article and measure its local
                // word frequency 
                var localFrequency = processArticle(task, ctx.reportAction);

                // Check what's up
                if(localFrequency == null)
                {
                    ctx.reportAction(MessageKind.Warning, "Failed to parse the article");
                    continue;
                }

                // Merge the local frequency into the global one
                mergeStatistics(ctx, localFrequency, task);

                // Report task completed
                ctx.reportAction(MessageKind.Info, String.Format("[10]Worker {0}: Parsed the article", ctx.id));
            }

            bool wasStopped = ctx.shouldStop.IsCancellationRequested;
            ctx.reportAction(wasStopped ? MessageKind.Warning : MessageKind.Info, 
                String.Format("Worker {0}: {1}", ctx.id, 
                    wasStopped ? "Terminated" : "Finished"
                ));
        }

        private static Dictionary<string, int> processArticle(ArticleHeader article, Action<MessageKind, string> doReport)
        {
            var result = new Dictionary<string, int>();

            // Ignore the failures
            try
            {
                // Dowload the article
                var client = new WebClient();
                var pageData = client.DownloadString(article.url);

                // Parse the HTML and extract the article's main content
                var html = new HtmlDocument();

                // Load the HTML string
                html.LoadHtml(pageData);

                var domRoot = html.DocumentNode;
                var content = domRoot.Descendants()
                    .Where(n => n.GetAttributeValue("class", "").Contains(Constants.SIGNATURE_CONTENT))
                    .Single();

                var textOnly = content.Descendants("p");

                // Combine the text from paragraphs
                StringBuilder sb = new StringBuilder(textOnly.Count());
                foreach(var node in textOnly)
                {
                    sb.Append(node.InnerText.ToLowerInvariant());
                    sb.Append(" ");
                }

                // Normalize the text
                var articleText = sb.ToString();
                articleText = HttpUtility.HtmlDecode(articleText.Replace("&nbsp;", " "));

                // Split the text into words
                string[] articleWords = articleText.Split(' ');

                // Update the local stats
                foreach(var word in articleWords)
                {
                    if (word == null || word.Length == 0)
                        continue;

                    if (!result.ContainsKey(word))
                    {
                        result[word] = 1;
                    }
                    else
                    {
                        result[word] = result[word] + 1;
                    }
                }
            }
            catch
            {
                return null;
            }

            return result;
        }

        private static void mergeStatistics(ArticleParserContext ctx, Dictionary<string, int> localStats, ArticleHeader header)
        {
            // First of all lock the global stats to
            // avoid collisions caused by the workers
            // running in parallel
            lock(ctx.globalStats)
            {
                ctx.reportAction(MessageKind.Info, String.Format("[10]Worker {0}: Merging stats...", ctx.id));

                // Run through the worker's local stas and merge it
                foreach (var wordInfo in localStats)
                {
                    string word = wordInfo.Key;

                    // Make sure the word exists
                    if (!ctx.globalStats.ContainsKey(word))
                    {
                        ctx.globalStats[word] = new WordStatsInfo();
                    }

                    // Add the local counter to the global one
                    ctx.globalStats[word].count = ctx.globalStats[word].count + wordInfo.Value;

                    // Add the article's reference
                    ctx.globalStats[word].articles.Add(header.url.ToString());
                }
            }
            ctx.reportAction(MessageKind.Info, String.Format("[10]Worker {0}: Stats merged", ctx.id));
        }
    }

}
