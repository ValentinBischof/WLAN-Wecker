using System;
using System.Net.Http;
using System.Web;
using System.Windows.Input;
using WLANWecker.Model;
using WLANWecker.View;
using Xamarin.Forms;

namespace WLANWecker.ViewModel
{
    [QueryProperty("alarm", "alarm")]
    [QueryProperty("ip", "ip")]
    public class AlarmViewModel : BindableObject
    {
        public ICommand SaveData { get; }
        public ICommand RemoveData { get; }
        public ICommand BackCommand { get; }
        public ICommand ShowWarning { get;  }

        public Alarm Alarm { get; set; }
        public string alarm
        {
            get
            {
                return Alarm.ToBytes();
            }
            set
            {
                Alarm = new Alarm();
                Alarm.FromBytes(value);
                Alarm.VariableChanged += OnUIChanged;
                OnPropertyChanged(nameof(Alarm));
            }
        }

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

        public AlarmViewModel()
        {
            SaveData = new Command(SaveAlarm);
            RemoveData = new Command(DeleteAlarm);
            BackCommand = new Command(ReturnToClockView);
            ShowWarning = new Command(ShowWarningExpertMode);
        }

        private async void ShowWarningExpertMode()
        {
            string msg = "Achtung, hierbei handelt es sich um ein Modus nur für geprobte Aufsteher. " +
                         "Statt den Wecker per Beschleunigung zu deaktiveren, " +
                         "werden hierbei Aufgaben auf dem Display gestellt über die aktuelle Ausrichtung des Gerätes";
            await Shell.Current.DisplayAlert("Hinweis", msg, "Verstanden");
        }

        private void OnUIChanged(object o, string name)
        {
            if (name.Equals(nameof(Alarm.repeat)))
                OnPropertyChanged(nameof(Alarm));

            Console.WriteLine("Hii!!!");
            Console.WriteLine(name);
        }

        private async void SaveAlarm()
        {
            string bytes = Alarm.ToBytes();
            string requestUrl = $@"http://{ip}/send/alarm?";

            var queryParam = HttpUtility.ParseQueryString(string.Empty);
            queryParam["data"] = bytes;
            string queryString = queryParam.ToString();

            HttpClient client = new HttpClient();

            Console.WriteLine(requestUrl + queryString);
            var response = await client.GetAsync(requestUrl + queryString);

            var responseString = await response.Content.ReadAsStringAsync();

            if (responseString != bytes)
            {
                await Application.Current.MainPage.DisplayAlert("Ew", "Ochje, die Antwort deine Anfrage, passt nicht nur Anfrange  Code: FM02 ", "Ok");
            }

            client.Dispose();
            await Shell.Current.GoToAsync($"{nameof(ClockView)}?ip={ip}");
        }

        private async void DeleteAlarm()
        {
            string requestUrl = $@"http://{ip}/remove/alarm?";

            var queryParam = HttpUtility.ParseQueryString(string.Empty);
            queryParam["id"] = Convert.ToString(Alarm.id);
            string queryString = queryParam.ToString();

            HttpClient client = new HttpClient();

            Console.WriteLine(requestUrl + queryString);
            var response = await client.GetAsync(requestUrl + queryString);

            await Shell.Current.GoToAsync($"{nameof(ClockView)}?ip={ip}");
        }

        private async void ReturnToClockView()
        {
            await Shell.Current.GoToAsync($"{nameof(ClockView)}?ip={ip}");
        }
    }
}
