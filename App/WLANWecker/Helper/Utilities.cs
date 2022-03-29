using System;
using System.Net.Http;
using System.Threading.Tasks;

namespace WLANWecker.Helper
{
    public static class Utilities
    {
        public static async Task<bool> CheckStatus(string connection)
        {
            bool found = false;

            HttpClient client = new HttpClient();
            string requestUrl = $@"http://{connection}/status";

            Console.WriteLine(requestUrl);
            try
            {
                var response = await client.GetStringAsync(requestUrl);
                if (response == "OK")
                    found = true;
            }
            catch (Exception)
            {
                found = false;
            }

            return found;
        }
    }
}
