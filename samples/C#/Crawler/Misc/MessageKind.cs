using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Crawler
{
    /**
        This enum is used to indicate the type of information messages
        occuring during the web site's crawling
    */
    public enum MessageKind
    {
        Trace,
        Info,
        Warning,
        Error,
        Result,
    };
}
