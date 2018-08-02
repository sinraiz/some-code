using HtmlAgilityPack;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Web;

namespace Crawler.Code
{
    class FrontPageParser
    {
        public FrontPageParser(string pageUrl)
        {
            try
            {
                _url = new Uri(pageUrl);
            }
            catch(Exception ex)
            {
                throw new Exception("The URL is invalid: " + ex.Message);
            }
            _client = new WebClient();
            _client.DownloadStringCompleted += pageDownloadCompleted;
        }


        public void beginParse()
        {
            // Only one download at a time
            if (_isDownloading)
            {
                throw new Exception("Another download is currently active");
            }

            // Initiate the asyncronous downloading
            _client.DownloadStringAsync(url);
        }

        private void pageDownloadCompleted(object sender, DownloadStringCompletedEventArgs e)
        {
            // Reset the busy flag
            _isDownloading = false;

            // Check what's going on
            bool isDownloadError = (e.Error != null) || e.Cancelled;
            string errorMsg = isDownloadError ? e.Error.Message : null;

            // Inform the callers
            if (onDownloaded != null)
            {
                onDownloaded(this, e.Result, !isDownloadError, errorMsg);
            }

            if(isDownloadError)
            {
                // Nothing else to do here
                return;
            }

            // Parse the page data and report the results
            var parseResults = parsePageInternal(e.Result);
            if(onParsed != null)
            {
                bool isError = (parseResults == null);
                errorMsg = isError ? "Failed to parse something" : null;
                onParsed(this, parseResults, !isError, errorMsg);
            }

        }

        private IEnumerable<ArticleHeader> parsePageInternal(string pageData)
        {
            // Prepare to parse
            var html = new HtmlDocument();
            
            // Load the HTML string
            html.LoadHtml(pageData);

            // Look for all the article posts
            var root = html.DocumentNode;
            var articleSummaries = root.Descendants()
                .Where(n => n.GetAttributeValue("class", "").Equals(Constants.SIGNATURE_POST));

            // If nothing is found return
            if(articleSummaries == null || articleSummaries.Count() == 0)
            {
                return null;
            }

            // Prepare the result collection
            var results = new List<ArticleHeader>(articleSummaries.Count());

            // Find data around each header
            int curArticleIndex = 0;
            foreach (var header in articleSummaries)
            {
                // Check if we are not beyond the limit
                if(curArticleIndex > Constants.MAX_ARTICLES)
                {
                    break;
                }

                // Try to parse an article header
                // If something goes wrong just report and move on
                try
                {
                    // get the title string
                    string articleTitle = header.InnerText;

                    // Clean it a bit
                    articleTitle = HttpUtility.HtmlDecode(articleTitle.Replace("&nbsp;", " "));

                    // Extract the hyper link
                    HtmlNode articleHref = header.Descendants("a")
                        .Single();

                    // Find the authors section
                    HtmlNode byAuthorsBlock = header.ParentNode.Descendants()
                        .Where(n => n.GetAttributeValue("class", "").Equals(Constants.SIGNATURE_AUTHORS))
                        .Single();

                    // Find the summary paragraph
                    HtmlNode excerptBlock = header.ParentNode.Descendants()
                        .Where(n => n.GetAttributeValue("class", "").Equals(Constants.SIGNATURE_SUMMARY))
                        .Single();

                    // If any of the above is missing, then ignore the whole block
                    if (articleHref == null || byAuthorsBlock == null || excerptBlock == null)
                    {
                        continue; // just move on
                    }

                    // We're ok so far

                    // Remove the time from the authors block and
                    // leave only their names
                    var timeNode = byAuthorsBlock.Descendants("time").Single();
                    if (timeNode != null)
                    {
                        byAuthorsBlock.RemoveChild(timeNode);
                    }

                    // Get the authors as text
                    string byAuthorsLine = byAuthorsBlock.InnerText
                        .Trim(new char []{ '\t', '\n', ' '});

                    if(byAuthorsLine.StartsWith("by "))
                    {
                        byAuthorsLine = byAuthorsLine.Substring(3);
                    }

                    // Extract the article's URL string
                    string articleUrl = articleHref.GetAttributeValue("href", "#");
                    
                    // The summary is simple
                    var summary = excerptBlock.InnerText;

                    // Sanitize it 
                    summary = HttpUtility.HtmlDecode(summary.Replace("&nbsp;", " "));

                    // Fill the article header
                    var articleHeader = new ArticleHeader()
                    {
                        title = articleTitle,
                        url = new Uri(articleUrl),
                        authors = byAuthorsLine,
                        summary = summary
                    };

                    // Push into the results
                    results.Add(articleHeader);
                }
                catch
                {
                    // Gracefully inform the caller that something went wrong
                    if(onReport != null)
                    {
                        onReport(MessageKind.Warning, String.Format("Failed to parse article #{0}", curArticleIndex));
                    }
                }

                // Move on
                curArticleIndex++;
            }

            // Return what we've got
            return results;
        }

        #region Events
        public event Action<FrontPageParser, string, bool, string> onDownloaded;
        public event Action<FrontPageParser, IEnumerable<ArticleHeader>, bool, string> onParsed;
        public event Action<MessageKind, string> onReport;
        #endregion

        #region Members
        private Uri _url;
        public Uri url
        {
            get
            {
                return _url;
            }
        }

        private WebClient _client;

        /**
            Identifies that the object is currently busy
            downloading the page
        */
        private bool _isDownloading = false;
        public bool isDownloading
        {
            get
            {
                return _isDownloading;
            }
        }

        #endregion
    }
}
