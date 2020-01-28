/* Desenvolvido por: Thainá França
 * Prefixos de retorno:
 *   - WARN: Erro com as informações da tabela ou da turma
 *   - SD: Informações ok, mas tabela não possui data da presença (continua a execução)
 *   - ERRO: Aluna não pagou o mensal ou não está cadastrada na turma
 *   - OK: Aluna pagou o mensal da turma
 *   - Sem prefixo: Cadastro de novo ID
 */

function fileId()
{
  return '1wla4FkO0GYtd7hPH3_WZNpB6N2pCKu4VHHXTeV5qq_o'; //ID da url da planilha
}

function doGet(e)
{
  Logger.log('Recebido: ' + JSON.stringify(e));
  var result = '';

  try
  {
    if (e.parameter == 'undefined') //Se vier sem parâmetros
      result = 'Sem Parâmetros';
    else
    {
      var uid = e.parameter['ID'].trim().replace(/^["']|['"]$/g, ""); //Retira espaços no início e fim e aspas
      if (uid != 'undefined')
      {
        var sheet_info = SpreadsheetApp.openById(fileId()).getSheetByName('Informações');
        result = getDados(sheet_info, uid);
      }
    }
    
    if (result.indexOf('OK') != 0)
    {
      Logger.log('Retorno: ' + result);
      MailApp.sendEmail('@email', '@assunto', Logger.getLog());
    }
  }
  catch(e)
  {
    Logger.log('ERRO: ' + e);
    MailApp.sendEmail('@email', 'Crash', Logger.getLog());
  }
  
  return ContentService.createTextOutput(result); //Retorna o que está no resultado
}

//Busca o nome da aluna e a turma que ela está
function getDados(sheet, id)
{
  var dados = sheet.getDataRange().getValues();
  var aluna = "", turma = "", tabela = "";
  var linha = 1, posAluna = 0;
  
  var hoje = new Date();
  var data_padrao = Utilities.formatDate(hoje, "America/Sao_Paulo", "MM/dd/yyyy");
  var data = Utilities.formatDate(hoje, "America/Sao_Paulo", "dd/MM/yyyy HH:mm");
  var dia_semana = hoje.getDay();
  var mes_ano = data.substring(3,10);
  var horario = hoje.getTime();
  
  for (linha; linha < dados.length; linha++)
  {
    if (dados[linha][0] == "") break;
	
    if (dados[linha][0].toUpperCase() == id.toUpperCase())
    {
      aluna = dados[linha][1].toUpperCase(); //Nome da aluna
      posAluna = linha + 1;
      Logger.log('Entrada da aluna: ' + aluna + ' - Data: ' + data);
      if (tabela != "") break;
    }
    
    if (dados[linha][6] == dia_semana)
    {
      if (dia_semana != "6")
      {
        turma = dados[linha][4].toUpperCase(); //Nome da turma
        tabela = dados[linha][5].toUpperCase() + " - " + mes_ano; //Tabela da turma
        Logger.log('Turma: ' + turma + " - Tabela de chamada: " + tabela);
        if (aluna != "") break;
      }
      else //Se for sábado, verifica horário
      {
        var horaTurma = new Date(data_padrao + " " + dados[linha][7]).getTime();
        var horaAnterior = horaTurma - (1000 * 30 * 60); //30 min antes
        var horaPosterior = horaTurma + (1000 * 60 * 60); //60 min depois
        if (horario >= horaAnterior && horario <= horaPosterior)
        {
          turma = dados[linha][4].toUpperCase(); //Nome da turma
          tabela = dados[linha][5].toUpperCase() + " - " + mes_ano; //Tabela da turma
          Logger.log('Turma: ' + turma + " - Tabela de chamada: " + tabela);
          if (aluna != "") break;
        }
      }
    }
  }
  
  if(aluna == "") //Se não encontrar a aluna, cadastra id e horário de entrada
  {
    sheet.getRange(linha, 1).setValue(id);
    sheet.getRange(linha, 3).setValue(data);
    return "Aluna nova cadastrada!";
  }
  else if (turma == "") //Se não encontrar a turma, salva horário de entrada
  {
    sheet.getRange(posAluna, 4).setValue(data);
    return "WARN: Turma não encontrada!";
  }
  else if (tabela != "") //Se obteve todos os dados...
  {
    var ret = checkPresenca(tabela, aluna, turma);
    if (ret.indexOf("SD.") == 0)
      sheet.getRange(posAluna, 4).setValue(data); //Se a data não estiver na tabela da turma, salva ao lado do nome da aluna
    
    return ret;
  }
}

//Checa o mensal e preenche a presença
function checkPresenca(sheet_nome, nome, turma)
{
  var hoje = Utilities.formatDate(new Date(), "America/Sao_Paulo", "dd/MM");
  var sheet = SpreadsheetApp.openById(fileId()).getSheetByName(sheet_nome);
  if(sheet == null)
    return "WARN: Tabela de chamada não encontrada!";
  
  var dados = sheet.getDataRange().getValues();
  var posTurma = 0;
  
  for (var i = 0; i < dados[0].length; i++)
  {
    if (dados[0][i].toUpperCase().indexOf(turma) >= 0)
    {
      posTurma = i + 1; //Coluna do nome das alunas
      break;
    }
  }
  
  var presenca = false;
  for (var i = 2; i < dados.length; i++)
  {
    if (dados[i][posTurma] == "")
    {
      sheet.getRange(i + 2, posTurma).setValue(nome);
      sheet.getRange(i + 2, posTurma + 1).setValue(hoje);
      return "ERRO: Aluna não encontrada na lista da turma!"; //Cria linha com nome da aluna e data
    }
    else if (dados[i][posTurma].toUpperCase() == nome) //Se for a linha da aluna...
	{
      for (var j = posTurma + 1; j < dados[1].length; j++)
      {
        if (dados[1][j] == hoje) //Assinala a presença
        {
          sheet.getRange(i + 1, j + 1).setValue("SIM");
          presenca = true;
        }
        else if (dados[1][j] == "Situação") //Verifica o mensal
        {
          var ret = "";
          if(!presenca) ret = "SD. "; //Sem data
          if (dados[i][j] == "PAGO" || dados[i][j] == "OK")
            return ret + "OK: Mensal pago!";
          else
            return ret + "ERRO: Mensal em aberto!";
        }
      }
	}
  }
}