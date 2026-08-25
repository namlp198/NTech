using System.Runtime.InteropServices;
using System.Text;

namespace NTech.NVision.Native;

internal sealed class NativeUtf8Allocation : IDisposable
{
    private IntPtr pointer;

    public NativeUtf8Allocation(string value)
    {
        ArgumentNullException.ThrowIfNull(value);
        pointer = Marshal.StringToCoTaskMemUTF8(value);
        View = new NativeUtf8View
        {
            Data = pointer,
            Length = (UIntPtr)Encoding.UTF8.GetByteCount(value),
        };
    }

    public NativeUtf8View View { get; }

    public void Dispose()
    {
        IntPtr owned = Interlocked.Exchange(ref pointer, IntPtr.Zero);
        if (owned != IntPtr.Zero)
        {
            Marshal.FreeCoTaskMem(owned);
        }
    }
}
