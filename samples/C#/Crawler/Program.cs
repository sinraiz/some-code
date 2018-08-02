using Crawler.Code;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Crawler
{
    class Program
    {
        private static void Crawler_onReport(MessageKind msgKind, string msgText)
        {
            // Based on the message kind choose the text color
            ConsoleColor color = ConsoleColor.White;

            switch (msgKind)
            {
                case MessageKind.Error:
                    color = ConsoleColor.Red;
                    break;
                case MessageKind.Info:
                case MessageKind.Trace:
                    color = ConsoleColor.White;
                    break;
                case MessageKind.Warning:
                    color = ConsoleColor.Yellow;
                    break;
                case MessageKind.Result:
                    color = ConsoleColor.Green;
                    break;
            };
            
            // Set the right text color
            Console.ForegroundColor = color;

            // If the we must print the columns then deal with it
            if (msgText.StartsWith("["))
            {
                string padding = msgText.Substring(1, msgText.IndexOf(']') - 1);
                int iPadding = int.Parse(padding);
                int iWidth = Console.WindowWidth - 1;

                // remove the padding info
                msgText = msgText.Substring(msgText.IndexOf(']') + 1);

                // Remove the line breaks
                // (actually no time to handle them correctly)
                msgText = msgText.Replace("\n", " ");

                // Extract and print the first line
                string line = msgText.Substring(0, Math.Min(msgText.Length, iWidth));
                Console.WriteLine(line);

                // All the next lines will be more narrow
                iWidth = Console.WindowWidth - iPadding - 1;

                // Index of the next line's text
                int curLinePos = line.Length;
                string paddingStub = new string(' ', iPadding);

                // Print line by line
                while (curLinePos > 0 && curLinePos < msgText.Length)
                {
                    line = msgText.Substring(curLinePos, Math.Min(msgText.Length - curLinePos, iWidth));
                    Console.WriteLine(paddingStub + line);
                    curLinePos += line.Length;
                }


            }
            else
            {
                // Simple print
                Console.WriteLine(msgText);
            }

            // Put the color back
            Console.ResetColor();
        }

        static void Main(string[] args)
        {
            SiteCrawler crawler = new SiteCrawler();
            crawler.onReport += Crawler_onReport;
            crawler.start();

            // Wait until the crawling ends
            while(!crawler.isDone)
            {
                // Meanwhile watch the keyboard
                if (Console.KeyAvailable)
                {
                    // Check what key was pressed
                    var key = Console.ReadKey(true);
                    if (key.Key == ConsoleKey.Escape)
                    {
                        crawler.stop();
                    }
                }
                else
                {
                    // No to waste the clock
                    System.Threading.Thread.Sleep(100);
                }
            }

            // Done with it
            Console.WriteLine("Press any key to continue");
            while(!Console.KeyAvailable)
            {
                System.Threading.Thread.Sleep(100);
            }
        }
    }
}
