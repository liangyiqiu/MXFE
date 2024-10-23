function [freq_range,fft_result]=fft_smooth(x,fs,n_samp,n_smooth)

n=n_samp/n_smooth;

df = fs/n; 
freq_range= (-fs/2:df:fs/2-df).'/1000000;

fft_temp=zeros(n_smooth,n);

for i=1:n_smooth
    fft_temp(i,:)=fftshift(20*log10(abs(fft(x((i-1)*n+1:i*n))/n)));
end

fft_result=mean(fft_temp);

end