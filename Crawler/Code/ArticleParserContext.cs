using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Crawler.Code
{
    /**
        This object is passed to each of the workers parsing the articles.
        It combines the cancelation event, the global tasks queue (articles) and
        also keeps the reference to the global frequency table which is to
        be updated by those workers
    */
    public class ArticleParserContext
    {
        // The worker's own index
        public int id { get; set; }

        // So that a worker knows when to stop
        public CancellationToken shouldStop { get; set; }

        // Shared set of tasks to take the work from
        public Queue<ArticleHeader> articles { get; set; }

        // Let him report some messages
        public Action<MessageKind, string> reportAction { get; set; }

        // The global shared stats storage
        public Dictionary<string, WordStatsInfo> globalStats { get; set; }
    }
}
