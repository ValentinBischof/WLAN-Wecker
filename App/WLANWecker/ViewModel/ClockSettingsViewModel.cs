using System;
using System.Net.Http;
using System.Web;
using WLANWecker.Model;
using WLANWecker.View;
using Xamarin.Forms;

namespace WLANWecker.ViewModel
{
   [QueryProperty("ip", "ip")]
   [QueryProperty("clock", "clock")]
    public class ClockSettingsViewModel : BindableObject
    {
        public Command SaveWiFiCommand { get; }

        private string _ip;
        public string ip
        {
            get
            {
                return _ip;
            }
            set
            {
                _ip = value;
            }
        }

        public string ssid { get; set; }
        public string pw { get; set; }


        public bool indicator { get; set; }
        public bool saveButton { get; set; }
 

        public ClockSettingsViewModel()
        {
            SaveWiFiCommand = new Command(SaveWiFiData);

            saveButton = true;
            indicator = false;
            OnPropertyChanged(nameof(Clock));
        }

        private async void SaveWiFiData(object obj)
        {
            string data = $"{ssid},{pw}";
            string requestUrl = $@"http://{ip}/data/connection?";

            var queryParam = HttpUtility.ParseQueryString(string.Empty);
            queryParam["data"] = data;
            string queryString = queryParam.ToString();

            HttpClient client = new HttpClient();

            Console.WriteLine(requestUrl + queryString);
            _ = client.GetAsync(requestUrl + queryString);


            indicator = true;
            saveButton = false;
            OnPropertyChanged(nameof(saveButton));
            OnPropertyChanged(nameof(indicator));

            await System.Threading.Tasks.Task.Delay(10000);
            client.Dispose();

            await Shell.Current.GoToAsync(nameof(ClockItemView));
        }
    }
}
