using WoteverLocalization;

namespace DiyFfbPedal
{
    // Translated UI text for controls built in code (same as {SLoc Key, 'Default'} in XAML).
    // Keys are listed with their English text in Language/DiyFfbPedal.resx for the translators.
    internal static class Loc
    {
        public static string T(string key, string english)
        {
            try
            {
                string text = SLoc.GetValue("DIYFFBPedalPlugin_" + key, english);
                return string.IsNullOrEmpty(text) ? english : text;
            }
            catch
            {
                return english;
            }
        }
    }
}
