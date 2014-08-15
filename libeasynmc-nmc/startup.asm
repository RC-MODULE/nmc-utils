//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//              ������� ����������� ����������� ���������������             //
//                                                                          //
//              ���������� ������� ����������                               //
//                                                                          //
//                          ��������� ���                                   //
//                                                                          //
//    $Revision:: 4     $      $Date:: 23.01.12 16:23   $					          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

// �����: �������� �.�.


global start  : label;	// ����������� ����� �����.
global _exit  : label;	// ������� exit.
global __exit : label;	// ������� _exit.


weak _sync : label; // ������� sync ��� �������� ������������ ���������

extern stop  : label; // ������� ������� ��������� ���������.

extern __main :label;
extern ctor : label;
extern dtor : label;
extern _easynmc_argc: word;
extern _easynmc_argv: word;


nobits stack
    STACK :long;     // ������ ������ �����
end stack;

begin text

         // Identification string
    CompilerString : long[4] =
            'NeuroMatrix(r) NM640x SDK v3.00';

        // ����� � ������ ��� �������� ���� �������� ���������
    result_code : word = -1;
    dummy : word = -1;

// ����� ����� ���������
<start>
    sp = STACK;								//  start +0
    delayed goto begin_prog with gr7=false;	//  start +2
    push gr7;								//  start +4,	argv =0
    push gr7;								//  start +5,	argc =0
				//  start +6 -�������, �� �������� ��������������� ���������						
<int_stop>
    goto stop;  // ������ �������. � gr7 � gr0 - ��� ��������
<_sync>         // ������� sync � �������� ������������ ���������
    return;

    // ������������������ ���� �� ������ ����������
    // start        - ������ ���������
    // start + 6    - ����� �������� ��������
    // start + 10   - ����� ������� sync ��� ������������ ���������

<begin_prog>
    call ctor;
    gr7 = [_easynmc_argv];     /* argv */
    push gr7;      
    gr7 = [_easynmc_argc];     /* argc */
    push gr7;     
    call __main;
    push ar7,gr7;

<terminate>
    call dtor;
    pop ar5,gr5;
    gr0 = gr5 with gr7 = gr5;
    [ result_code ] = gr7;
    goto int_stop;

<_exit>
    sp -= 2;
    goto terminate;

<__exit>
    sp -= 2;
    pop ar5,gr5;
    gr0 = gr5 with gr7 = gr5;
    [ result_code ] = gr7;
    goto int_stop;

end text;
